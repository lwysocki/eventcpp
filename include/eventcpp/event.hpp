/**
 * \brief	C++ event implementation
 * \author	Lukasz Wysocki
 */

#ifndef EVENTCPP_EVENT_HPP
#define EVENTCPP_EVENT_HPP

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_set>

namespace event
{
    namespace details
    {
        template<typename TRet, typename ...Args> class invokable_abstract;

        template<typename TRet, typename ...Args>
        inline bool operator!= (const invokable_abstract<TRet, Args...>& lhs,
                                const invokable_abstract<TRet, Args...>& rhs)
        {
            return lhs.get_func_ptr() != rhs.get_func_ptr() && lhs.get_obj_ptr() != rhs.get_obj_ptr();
        }

        template<typename TRet, typename ...Args>
        inline bool operator== (const invokable_abstract<TRet, Args...>& lhs,
                                const invokable_abstract<TRet, Args...>& rhs)
        {
            return lhs.get_func_ptr() == rhs.get_func_ptr() && lhs.get_obj_ptr() == rhs.get_obj_ptr();
        }

        template<typename TRet, typename ...Args>
        class invokable_abstract
        {
        public:
            virtual TRet operator() (Args&&...) = 0;
            [[nodiscard]] virtual std::string get_func_ptr() const = 0;
            [[nodiscard]] virtual uintptr_t get_obj_ptr() const = 0;
            [[nodiscard]] virtual size_t get_hash() const = 0;
        };

        template<typename TRet, typename ...Args>
        class invokable_func : public invokable_abstract<TRet, Args...>
        {
            using TFuncPtr = typename std::add_pointer_t<TRet(Args...)>;

        public:
            explicit invokable_func(TFuncPtr func) : _func(func)
            {
            }

            TRet operator() (Args&&... args) override
            {
                if (_func == nullptr)
                    throw std::bad_function_call();

                return std::invoke(_func, std::forward<Args>(args)...);
            }

            [[nodiscard]] std::string get_func_ptr() const override
            {
                const auto tmp = static_cast<const char*>(static_cast<const void*>(&_func));
                std::string tmpstr(tmp);

                return tmpstr;
            }

            [[nodiscard]] uintptr_t get_obj_ptr() const override
            {
                return reinterpret_cast<uintptr_t>(nullptr);
            }

            [[nodiscard]] size_t get_hash() const override
            {
                return std::hash<std::string>{}(get_func_ptr());
            }

        private:
            TFuncPtr _func;
        };

        template<typename TRet, typename TClass, typename ...Args>
        class invokable_member : public invokable_abstract<TRet, Args...>
        {
            using TFuncPtr = TRet(TClass::*) (Args...);
            using TClassRef = typename std::add_lvalue_reference_t<TClass>;

        public:
            invokable_member(TFuncPtr func, TClassRef obj) : _func(func), _obj(obj)
            {
            }

            TRet operator() (Args&&... args) override
            {
                if (_func == nullptr || &_obj == nullptr)
                    throw std::bad_function_call();

                return std::invoke(_func, _obj, std::forward<Args>(args)...);
            }

            [[nodiscard]] std::string get_func_ptr() const override
            {
                const auto tmp = static_cast<const char*>(static_cast<const void*>(&_func));
                std::string tmpstr(tmp);

                return tmpstr;
            }

            [[nodiscard]] uintptr_t get_obj_ptr() const override
            {
                return reinterpret_cast<uintptr_t>(&_obj);
            }

            [[nodiscard]] size_t get_hash() const override
            {
                size_t func_hash = std::hash<std::string>{}(get_func_ptr());
                size_t obj_hash = std::hash<uintptr_t>{}(get_obj_ptr());

                return func_hash ^ (obj_hash + 0x9e3779b9 + (func_hash << 6) + (func_hash >> 2));
            }

        private:
            TFuncPtr _func;
            TClassRef _obj;
        };
    } // namespace details

    template<typename TFunc> class event;

    /**
     * \brief A container class for subscribers to be notified
     *
     * \tparam TRet type returned by a callback of a subscriber
     * \tparam Args arguments accepted by a callback of a subscriber
     */
    template<typename TRet, typename ...Args>
    class event<TRet(Args...)>
    {
        using TFunc = TRet(Args...);
        using TFuncPtr = typename std::add_pointer_t<TFunc>;
        using TInvokable = details::invokable_abstract<TRet, Args...>;

    public:
        /**
         * \brief The function call operator for notifying subscribers
         *
         * \param args arguments that will be passed to subscribed callbacks
         * \return TRet type returned by a callback of a subscriber
         */
        template<typename _TRet = TRet>
        std::enable_if_t<!std::is_same_v<_TRet, void>, _TRet>
            operator() (Args&&... args)
        {
            _TRet ret;

            for (auto invokable : _invokables)
            {
                ret = std::invoke(*invokable, std::forward<Args>(args)...);
            }

            return ret;
        }

        template<typename _TRet = TRet>
        std::enable_if_t<std::is_same_v<_TRet, void>, _TRet>
            operator() (Args&&... args)
        {
            for (auto invokable : _invokables)
            {
                std::invoke(*invokable, std::forward<Args>(args)...);
            }
        }

        /**
         * \brief Attach function callback
         */
        void attach(TFuncPtr func)
        {
            _invokables.emplace(std::make_shared<details::invokable_func<TRet, Args... >>(func));
        }

        /**
         * \brief Attach member function callback
         */
        template<typename TClass>
        void attach(TRet(TClass::* func) (Args...), TClass& obj)
        {
            _invokables.emplace(std::make_shared<details::invokable_member<TRet, TClass, Args...>>(func, obj));
        }

        /**
         * \brief Attach member function callback
         */
        template<typename TClass>
        void attach(TRet(TClass::* func) (Args...), TClass* obj)
        {
            attach(func, *obj);
        }

        /**
         * \brief Attach member function callback
         */
        template<typename TBase, typename TClass>
        void attach(TRet(TBase::* func) (Args...), TClass& obj)
        {
            _invokables.emplace(std::make_shared<details::invokable_member<TRet, TClass, Args...>>(func, obj));
        }

        /**
         * \brief Attach member function callback
         */
        template<typename TBase, typename TClass>
        void attach(TRet(TBase::* func) (Args...), TClass* obj)
        {
            attach(func, *obj);
        }

        /**
         * \brief Dettach function callback
         */
        void detach(TFuncPtr func)
        {
            details::invokable_func<TRet, Args...> invokable(func);
            remove(invokable);
        }

        /**
         * \brief Dettach member function callback
         */
        template<typename TClass>
        void detach(TFunc TClass::* func, TClass& obj)
        {
            details::invokable_member<TRet, TClass, Args...> invokable(func, obj);
            remove(invokable);
        }

        /**
         * \brief Dettach member function callback
         */
        template<typename TClass>
        void detach(TFunc TClass::* func, TClass* obj)
        {
            detach(func, *obj);
        }

        /**
         * \brief Dettach member function callback
         */
        template<typename TBase, typename TClass>
        void detach(TFunc TBase::* func, TClass& obj)
        {
            details::invokable_member<TRet, TClass, Args...> invokable(func, obj);
            remove(invokable);
        }

        /**
         * \brief Dettach member function callback
         */
        template<typename TBase, typename TClass>
        void detach(TFunc TBase::* func, TClass* obj)
        {
            detach(func, *obj);
        }

    private:
        struct InvokableHasher
        {
            size_t operator()(const std::shared_ptr<TInvokable>& invokable) const
            {
                return invokable.get()->get_hash();
            }
        };

        struct InvokableComparator
        {
            bool operator()(const std::shared_ptr<TInvokable>& lhs, const std::shared_ptr<TInvokable>& rhs) const
            {
                return (*lhs) != (*rhs);
            }
        };

        std::unordered_set<std::shared_ptr<TInvokable>, InvokableHasher, InvokableComparator> _invokables;

        void remove(const TInvokable& invokable)
        {
            auto it = std::find_if(_invokables.begin(), _invokables.end(), [&](const auto &element)
            {
                return (*element) == invokable;
            });

            if (it != _invokables.end())
            {
                _invokables.erase(it);
            }
        }
    };
} // namespace event

#endif // !EVENTCPP_EVENT_HPP
