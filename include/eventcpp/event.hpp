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
#include <utility>

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

        template<typename TRet, typename TObj, typename ...Args>
        class invokable_member : public invokable_abstract<TRet, Args...>
        {
            using TFuncPtr = TRet(TObj::*) (Args...);
            using TObjRef = typename std::add_lvalue_reference_t<TObj>;

        public:
            invokable_member(TFuncPtr func, TObjRef obj) : _func(func), _obj(obj)
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
            TObjRef _obj;
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
        using TInvokable = details::invokable_abstract<TRet, Args...>;

    public:
        /**
         * \brief Invokes all subscribed callbacks with the provided arguments.
         *
         * This operator forwards arguments to every subscriber in the internal collection.
         * Because subscribers are stored in an unordered collection, the execution
         * order is not guaranteed.
         *
         * \tparam CallArgs arguments accepted by a callback
         * \param args Universal reference to arguments forwarded to subscribers.
         * \return If TRet is void, returns nothing. Otherwise, returns the TRet value
         * produced by the final subscriber visited during iteration. If no subscribers
         * exist, returns a default-initialized TRet.
         */
        template<typename... CallArgs>
        auto operator() (CallArgs&&... args)
        {
            if constexpr (std::is_same_v<TRet, void>)
            {
                for (auto invokable : _invokables)
                {
                    std::invoke(*invokable, std::forward<Args>(args)...);
                }
            }
            else
            {
                TRet return_value{};

                for (auto invokable : _invokables)
                {
                    return_value = std::invoke(*invokable, std::forward<Args>(args)...);
                }

                return return_value;
            }
        }

        /**
         * \brief Universal attach for lambdas, function pointers, and functors.
         *
         * \param func The callable object to subscribe to the event.
         */
        template<typename F>
        requires std::invocable<F, Args...> &&
                 (!std::is_member_function_pointer_v<std::decay_t<F>>)
        void attach(F&& func)
        {
            _invokables.emplace(std::make_shared<details::invokable_func<TRet, Args...>>(std::forward<F>(func)));
        }

        /**
         * \brief Attach for member functions.
         *
         * This overload specifically targets member function pointers via object reference.
         *
         * \param member_ptr Pointer to the member function.
         * \param obj Object passed by reference to invoke member function.
         */
        template<typename M, typename T>
        requires std::is_member_function_pointer_v<M>
        void attach(M member_ptr, T& obj)
        {
            _invokables.emplace(std::make_shared<details::invokable_member<TRet, T, Args...>>(member_ptr, obj));
        }

        /**
         * \brief Attach for member functions.
         *
         * This overload specifically targets member function pointers via object pointer.
         *
         * \param member_ptr Pointer to the member function.
         * \param obj Object passed by pointer to invoke member function.
         */
        template<typename M, typename T>
        requires std::is_member_function_pointer_v<M>
        void attach(M member_ptr, T* obj)
        {
            attach(member_ptr, *obj);
        }

        /**
         * \brief Universal detach for lambdas, function pointers, and functors.
         *
         * \param func The callable object to unsubscribe from the event.
         */
        template<typename F>
        requires std::invocable<F, Args...> &&
                 (!std::is_member_function_pointer_v<std::decay_t<F>>)
        void detach(F&& func)
        {
            details::invokable_func<TRet, Args...> invokable(std::forward<F>(func));
            remove(invokable);
        }

        /**
         * \brief Detach for member functions.
         *
         * This overload specifically targets member function pointers via object reference.
         *
         * \param member_ptr Pointer to the member function.
         * \param obj Object passed by reference to invoke member function.
         */
        template<typename M, typename T>
        requires std::is_member_function_pointer_v<M>
        void detach(M member_ptr, T& obj)
        {
            details::invokable_member<TRet, T, Args...> invokable(member_ptr, obj);
            remove(invokable);
        }

        /**
         * \brief Detach for member functions.
         *
         * This overload specifically targets member function pointers via object pointer.
         *
         * \param member_ptr Pointer to the member function.
         * \param obj Object passed by pointer to invoke member function.
         */
        template<typename M, typename T>
        requires std::is_member_function_pointer_v<M>
        void detach(M member_ptr, T* obj)
        {
            detach(member_ptr, *obj);
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
