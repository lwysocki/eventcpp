/**
 * \brief	C++ event implementation
 * \author	Lukasz Wysocki
 */

#ifndef EVENTCPP_EVENT_HPP
#define EVENTCPP_EVENT_HPP

#include <algorithm>
#include <functional>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace event
{
    namespace details
    {
        template<typename TRet, typename ...Args>
        class invokable_abstract
        {
        public:
            virtual TRet operator() (Args&&...) = 0;
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

        private:
            TFuncPtr _func;
            TObjRef _obj;
        };
    } // namespace details

    template<typename TFunc> class event;
    template<typename TFunc> class connection;

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
        event()
        {
            _link = std::make_shared<link>(this);
        }

        event(event<TRet(Args...)>&& other) noexcept : _link(std::move(other._link)), _invokables(std::move(other._invokables))
        {
        }

        event<TRet(Args...)>& operator=(event<TRet(Args...)>&& other) noexcept
        {
            if (this == &other) return *this;

            _link = std::move(other._link);
            _invokables = std::move(other._invokables);

            if (_link)
                _link->_event_ptr = this;

            return *this;
        }

        ~event()
        {
            if (_link)
                _link->_event_ptr = nullptr;
        }

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
                for (const auto& invokable : _invokables)
                {
                    std::invoke(*invokable.second, std::forward<Args>(args)...);
                }
            }
            else
            {
                TRet return_value{};

                for (const auto& invokable : _invokables)
                {
                    return_value = std::invoke(*invokable.second, std::forward<Args>(args)...);
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
        auto attach(F&& func)
        {
            connection<TRet(Args...)> connection(_link->weak_from_this(), std::make_shared<details::invokable_func<TRet, Args...>>(std::forward<F>(func)));
            _invokables.emplace(connection._token.get(), connection._token);

            return connection;
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
        auto attach(M member_ptr, T& obj)
        {
            connection<TRet(Args...)> connection(_link->weak_from_this(), std::make_shared<details::invokable_member<TRet, T, Args...>>(member_ptr, obj));
            _invokables.emplace(connection._token.get(), connection._token);

            return connection;
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
        auto attach(M member_ptr, T* obj)
        {
            return attach(member_ptr, *obj);
        }

        /**
         * \brief Detach subscriber.
         *
         * \param connection Connection object used to identify subscriber.
         */
        void detach(const connection<TRet(Args...)>& connection)
        {
            _invokables.erase(connection._token.get());
        }

    private:
        friend connection<TRet(Args...)>;

        class link : public std::enable_shared_from_this<link>
        {
        public:
            link(event<TRet(Args...)>* event_ptr) : _event_ptr(event_ptr)
            {
            }

            void detach(const connection<TRet(Args...)>& connection)
            {
                _event_ptr->detach(connection);
            }

        private:
            friend event<TRet(Args...)>;

            event<TRet(Args...)>* _event_ptr;
        };

        std::shared_ptr<link> _link;
        std::unordered_map<TInvokable*, std::shared_ptr<TInvokable>> _invokables;
    };

    template<typename TRet, typename ...Args>
    class connection<TRet(Args...)>
    {
    public:
        connection() = default;

        connection(connection<TRet(Args...)>&& other) noexcept : _link(std::move(other._link)), _token(std::move(other._token))
        {
        }

        connection<TRet(Args...)>& operator=(connection<TRet(Args...)>&& other) noexcept
        {
            if (this == &other) return *this;

            _link = std::move(other._link);
            _token = std::move(other._token);
            other._token.reset();

            return *this;
        }

        ~connection() = default;

        void disconnect()
        {
            if (_token == nullptr) return;

            if (auto shared = _link.lock())
            {
                shared->detach(*this);
                _token.reset();
            }
        }

    private:
        friend event<TRet(Args...)>;

        connection(std::weak_ptr<typename event<TRet(Args...)>::link> link, std::shared_ptr<details::invokable_abstract<TRet, Args...>> token) : _link(link), _token(token)
        {
        }

        std::weak_ptr<typename event<TRet(Args...)>::link> _link;
        std::shared_ptr<details::invokable_abstract<TRet, Args...>> _token;
    };
} // namespace event

#endif // !EVENTCPP_EVENT_HPP
