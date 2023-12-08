#pragma once

#include <hex/api/imhex_api.hpp>
#include <hex/api/event_manager.hpp>

#include <map>
#include <utility>

namespace hex {

    namespace prv {
        class Provider;
    }

    template<typename T>
    class PerProvider {
    public:
        PerProvider() { this->onCreate(); }
        PerProvider(const PerProvider&) = delete;
        PerProvider(PerProvider&&) = delete;
        PerProvider& operator=(const PerProvider&) = delete;
        PerProvider& operator=(PerProvider &&) = delete;

        PerProvider(T data) : m_data({ { ImHexApi::Provider::get(), std::move(data) } }) { this->onCreate(); }

        ~PerProvider() { this->onDestroy(); }

        T* operator->() {
            return &this->get();
        }

        const T* operator->() const {
            return &this->get();
        }

        T& get(prv::Provider *provider = ImHexApi::Provider::get()) {
            return this->m_data[provider];
        }

        const T& get(prv::Provider *provider = ImHexApi::Provider::get()) const {
            return this->m_data[provider];
        }

        void set(const T &data, prv::Provider *provider = ImHexApi::Provider::get()) {
            this->m_data[provider] = data;
        }

        void set(T &&data, prv::Provider *provider = ImHexApi::Provider::get()) {
            this->m_data[provider] = std::move(data);
        }

        T& operator*() {
            return this->get();
        }

        const T& operator*() const {
            return this->get();
        }

        PerProvider& operator=(const T &data) {
            this->set(data);
            return *this;
        }

        PerProvider& operator=(T &&data) {
            this->set(std::move(data));
            return *this;
        }

        operator T&() {
            return this->get();
        }

    private:
        void onCreate() {
            EventProviderOpened::subscribe(this, [this](prv::Provider *provider) {
                this->m_data.emplace(provider, T());
            });

            EventProviderDeleted::subscribe(this, [this](prv::Provider *provider){
                this->m_data.erase(provider);
            });

            EventImHexClosing::subscribe(this, [this] {
                this->m_data.clear();
            });

            // Moves the data of this PerProvider instance from one provider to another
            MovePerProviderData::subscribe(this, [this](prv::Provider *from, prv::Provider *to) {
                // Get the value from the old provider, (removes it from the map)
                auto node = m_data.extract(from);

                // Ensure the value existed
                if (node.empty()) return;

                // Delete the value from the new provider, that we want to replace
                this->m_data.erase(to);

                // Re-insert it with the key of the new provider
                node.key() = to;
                this->m_data.insert(std::move(node));
            });
        }

        void onDestroy() {
            EventProviderOpened::unsubscribe(this);
            EventProviderDeleted::unsubscribe(this);
            EventImHexClosing::unsubscribe(this);
        }

    private:
        std::map<prv::Provider *, T> m_data;
    };

}