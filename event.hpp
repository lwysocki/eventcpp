/**
 * \brief	C++ Events
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

namespace Event
{
    namespace Details
    {
        template<typename TRet, typename ...Args> class InvokableAbstract;

        template<typename TRet, typename ...Args>
        inline bool operator!= (const InvokableAbstract<TRet, Args...>& lhs,
            const InvokableAbstract<TRet, Args...>& rhs)
        {
            return lhs.GetFuncPtr() != rhs.GetFuncPtr() && lhs.GetObjPtr() != rhs.GetObjPtr();
        }

        template<typename TRet, typename ...Args>
        class InvokableAbstract
        {
        public:
            virtual TRet operator() (Args&&...) = 0;
            virtual std::string GetFuncPtr() const = 0;
            virtual uintptr_t GetObjPtr() const = 0;
        };

        template<typename TRet, typename ...Args>
        class InvokableFunc : public InvokableAbstract<TRet, Args...>
        {
            using _TFuncPtr = typename std::add_pointer<TRet(Args...)>::type;

        public:
            InvokableFunc(_TFuncPtr func) : _func(func)
            {
            }

            TRet operator() (Args&&... args) override
            {
                if (_func == nullptr)
                    throw std::bad_function_call();

                return std::invoke(_func, std::forward<Args>(args)...);
            }

            std::string GetFuncPtr() const override
            {
                auto tmp = static_cast<const char*>(static_cast<const void*>(&_func));
                std::string tmpstr(tmp);

                return tmpstr;
            }

            uintptr_t GetObjPtr() const override
            {
                return reinterpret_cast<uintptr_t>(nullptr);
            }

        private:
            _TFuncPtr _func;
        };

        template<typename TRet, typename TClass, typename ...Args>
        class InvokableMember : public InvokableAbstract<TRet, Args...>
        {
            using _TFuncPtr = TRet(TClass::*) (Args...);
            using _TClassRef = typename std::add_lvalue_reference<TClass>::type;

        public:
            InvokableMember(_TFuncPtr func, _TClassRef obj) : _func(func), _obj(obj)
            {
            }

            TRet operator() (Args&&... args) override
            {
                if (_func == nullptr || &_obj == nullptr)
                    throw std::bad_function_call();

                return std::invoke(_func, _obj, std::forward<Args>(args)...);
            }

            std::string GetFuncPtr() const override
            {
                auto tmp = static_cast<const char*>(static_cast<const void*>(&_func));
                std::string tmpstr(tmp);

                return tmpstr;
            }

            uintptr_t GetObjPtr() const override
            {
                return reinterpret_cast<uintptr_t>(&_obj);
            }

        private:
            _TFuncPtr _func;
            _TClassRef _obj;
        };
    }

    template<typename TFunc> class Event;

    template<typename TRet, typename ...Args>
    class Event<TRet(Args...)>
    {
        using _TFunc = TRet(Args...);
        using _TFuncPtr = typename std::add_pointer<_TFunc>::type;
        using _TInvokable = Details::InvokableAbstract<TRet, Args...>;

    public:
        template<typename ..._Args>
        TRet operator() (_Args&&... args)
        {
            TRet ret;

            for (auto invokable : _invokables)
            {
                ret = std::invoke(*invokable, std::forward<Args>(args)...);
            }

            return ret;
        }

        void Attach(_TFuncPtr func)
        {
            std::shared_ptr<Details::InvokableFunc<TRet, Args...>> ptr(new Details::InvokableFunc<TRet, Args... >(func));
            _invokables.push_back(ptr);
        }

        template<typename TClass>
        void Attach(TRet(TClass::* func) (Args...), TClass& obj)
        {
            std::shared_ptr<Details::InvokableMember<TRet, TClass, Args...>> ptr(new Details::InvokableMember<TRet, TClass, Args...>(func, obj));
            _invokables.push_back(ptr);
        }

        template<typename TClass>
        void Attach(TRet(TClass::* func) (Args...), TClass* obj)
        {
            Attach(func, *obj);
        }

        template<typename TBase, typename TClass>
        void Attach(TRet(TBase::* func) (Args...), TClass& obj)
        {
            std::shared_ptr<Details::InvokableMember<TRet, TClass, Args...>> ptr(new Details::InvokableMember<TRet, TClass, Args...>(func, obj));
            _invokables.push_back(ptr);
        }

        template<typename TBase, typename TClass>
        void Attach(TRet(TBase::* func) (Args...), TClass* obj)
        {
            Attach(func, *obj);
        }

        void Detach(_TFuncPtr func)
        {
            InvokableFunc<TRet, Args...> invokable(func);
            Remove(invokable);
        }

        template<typename TClass>
        void Detach(_TFunc TClass::* func, TClass& obj)
        {
            Details::InvokableMember<TRet, TClass, Args...> invokable(func, obj);
            Remove(invokable);
        }

        template<typename TClass>
        void Detach(_TFunc TClass::* func, TClass* obj)
        {
            Detach(func, *obj);
        }

        template<typename TBase, typename TClass>
        void Detach(_TFunc TBase::* func, TClass& obj)
        {
            InvokableMember<TRet, TClass, Args...> invokable(func, obj);
            Remove(invokable);
        }

        template<typename TBase, typename TClass>
        void Detach(_TFunc TBase::* func, TClass* obj)
        {
            Detach(func, *obj);
        }

    private:
        std::list<std::shared_ptr<_TInvokable>> _invokables;

        void Remove(const _TInvokable& invokable)
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
