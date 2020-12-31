/**
 * \brief	C++ event implementation
 * \author	Lukasz Wysocki
 */

#ifndef __event__
#define __event__

#include <cstdint>
#include <functional>
#include <memory>
#include <list>
#include <type_traits>
#include <string>

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
        class invokable_abstract
        {
        public:
            virtual TRet operator() (Args&&...) = 0;
            virtual std::string get_func_ptr() const = 0;
            virtual uintptr_t get_obj_ptr() const = 0;
        };

        template<typename TRet, typename ...Args>
        class invokable_func : public invokable_abstract<TRet, Args...>
        {
            using _TFuncPtr = typename std::add_pointer<TRet(Args...)>::type;

        public:
            invokable_func(_TFuncPtr func) : _func(func)
            {
            }

            TRet operator() (Args&&... args) override
            {
                if (_func == nullptr)
                    throw std::bad_function_call();

                return std::invoke(_func, std::forward<Args>(args)...);
            }

            std::string get_func_ptr() const override
            {
                auto tmp = static_cast<const char*>(static_cast<const void*>(&_func));
                std::string tmpstr(tmp);

                return tmpstr;
            }

            uintptr_t get_obj_ptr() const override
            {
                return reinterpret_cast<uintptr_t>(nullptr);
            }

        private:
            _TFuncPtr _func;
        };

        template<typename TRet, typename TClass, typename ...Args>
        class invokable_member : public invokable_abstract<TRet, Args...>
        {
            using _TFuncPtr = TRet(TClass::*) (Args...);
            using _TClassRef = typename std::add_lvalue_reference<TClass>::type;

        public:
            invokable_member(_TFuncPtr func, _TClassRef obj) : _func(func), _obj(obj)
            {
            }

            TRet operator() (Args&&... args) override
            {
                if (_func == nullptr || &_obj == nullptr)
                    throw std::bad_function_call();

                return std::invoke(_func, _obj, std::forward<Args>(args)...);
            }

            std::string get_func_ptr() const override
            {
                auto tmp = static_cast<const char*>(static_cast<const void*>(&_func));
                std::string tmpstr(tmp);

                return tmpstr;
            }

            uintptr_t get_obj_ptr() const override
            {
                return reinterpret_cast<uintptr_t>(&_obj);
            }

        private:
            _TFuncPtr _func;
            _TClassRef _obj;
        };
    }

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
        using _TFunc = TRet(Args...);
        using _TFuncPtr = typename std::add_pointer<_TFunc>::type;
        using _TInvokable = details::invokable_abstract<TRet, Args...>;

    public:
        /**
         * \brief The function call operator for notifying subscribers
         *
         * \param args arguments that will be passed to subscribed callbacks
         * \return TRet type returned by a callback of a subscriber
         */
        template<typename _TRet = TRet>
        std::enable_if_t<!std::is_same<_TRet, void>::value, _TRet>
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
        std::enable_if_t<std::is_same<_TRet, void>::value, _TRet>
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
        void attach(_TFuncPtr func)
        {
            std::shared_ptr<details::invokable_func<TRet, Args...>> ptr(new details::invokable_func<TRet, Args... >(func));
            _invokables.push_back(ptr);
        }

        /**
         * \brief Attach member function callback
         */
        template<typename TClass>
        void attach(TRet(TClass::* func) (Args...), TClass& obj)
        {
            std::shared_ptr<details::invokable_member<TRet, TClass, Args...>> ptr(new details::invokable_member<TRet, TClass, Args...>(func, obj));
            _invokables.push_back(ptr);
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
            std::shared_ptr<details::invokable_member<TRet, TClass, Args...>> ptr(new details::invokable_member<TRet, TClass, Args...>(func, obj));
            _invokables.push_back(ptr);
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
        void detach(_TFuncPtr func)
        {
            details::invokable_func<TRet, Args...> invokable(func);
            remove(invokable);
        }

        /**
         * \brief Dettach member function callback
         */
        template<typename TClass>
        void detach(_TFunc TClass::* func, TClass& obj)
        {
            details::invokable_member<TRet, TClass, Args...> invokable(func, obj);
            remove(invokable);
        }

        /**
         * \brief Dettach member function callback
         */
        template<typename TClass>
        void detach(_TFunc TClass::* func, TClass* obj)
        {
            detach(func, *obj);
        }

        /**
         * \brief Dettach member function callback
         */
        template<typename TBase, typename TClass>
        void detach(_TFunc TBase::* func, TClass& obj)
        {
            details::invokable_member<TRet, TClass, Args...> invokable(func, obj);
            remove(invokable);
        }

        /**
         * \brief Dettach member function callback
         */
        template<typename TBase, typename TClass>
        void detach(_TFunc TBase::* func, TClass* obj)
        {
            detach(func, *obj);
        }

    private:
        std::list<std::shared_ptr<_TInvokable>> _invokables;

        void remove(const _TInvokable& invokable)
        {
            auto it = _invokables.begin();

            while (it != _invokables.end() && (*(*it)) != invokable)
            {
                it++;
            }

            if (it != _invokables.end())
            {
                _invokables.erase(it);
            }
        }
    };
}

#endif // !__event__
