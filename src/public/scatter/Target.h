/**
 * scatter
 * EventConfig.h
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef SCATTER_TARGET_H
#define SCATTER_TARGET_H

#include <string>
#include "Message.h"
#include <json.hpp>
#include <dlfcn.h>

namespace wss {
namespace event {

/// \brief Available:
/// postback: PostbackTarget
///     "url": "http://example.com/postback",
class Target {
 public:
    // DEFS
    typedef Target *(CreateFunc)(const char *);
    typedef void    (ReleaseFunc)(Target *);


    /// \brief Accept json config of entire target object
    /// \param config
    explicit Target(const nlohmann::json &config) :
        m_config(config),
        m_validState(true),
        m_errorMessage("") {
    }

    /// \brief Send event to entire target
    /// \param payload Payload to send
    /// \param error if method returned false, error will contains error message
    /// \return true if sending complete
    virtual bool send(const wss::MessagePayload &payload, std::string &error) = 0;
    virtual std::string getType() = 0;

    /// \brief Check target is in valid state
    /// \return valid state of target object
    bool isValid() const {
        return m_validState;
    }

    /// \brief If target is not valid, message will be available
    /// \return error message
    std::string getErrorMessage() const {
        return m_errorMessage;
    }

    void addFallback(std::shared_ptr<wss::event::Target> &&fallbackTarget) {
        fallbackTargets.push_back(fallbackTarget);
    }

    const std::vector<std::shared_ptr<wss::event::Target>> getFallbacks() const {
        return fallbackTargets;
    }

 protected:
    /// \brief Set error message to read when object is invalid state. Mark object as in invalid state/
    /// \param msg
    void setErrorMessage(const std::string &msg) {
        m_errorMessage = msg;
        m_validState = false;
    }

    /// \brief Set error message to read when object is invalid state. Mark object as in invalid state/
    /// \param msg rvalue string
    void setErrorMessage(std::string &&msg) {
        m_errorMessage = std::move(msg);
        m_validState = false;
    }

    /// \brief Appends to existing message new message separated by new line. If message did not set before, it will be a single message.
    /// Mark object as in invalid state/
    /// \param msg rvalue error message
    void appendErrorMessage(std::string &&msg) {
        if (m_errorMessage.empty()) {
            setErrorMessage(std::move(msg));
            return;
        }

        m_errorMessage = fmt::format("{0}\n{1}", m_errorMessage, msg);
    }

    /// \brief Appends to existing message new message separated by new line. If message did not set before, it will be a single message.
    /// Mark object as in invalid state/
    /// \param msg lvalue error message
    void appendErrorMessage(const std::string &msg) {
        if (m_errorMessage.empty()) {
            setErrorMessage(msg);
            return;
        }

        m_errorMessage = fmt::format("{0}\n{1}", m_errorMessage, msg);
    }
 private:
    nlohmann::json m_config;
    bool m_validState;
    std::string m_errorMessage;
    std::vector<std::shared_ptr<wss::event::Target>> fallbackTargets;
};

extern "C" SCATTER_EXPORT Target *target_create(const nlohmann::json &config);
extern "C" SCATTER_EXPORT void target_release(Target *target);


/// \brief RAII wrapper for dynamic load shared libs
class TargetLoader final {
 public:
    TargetLoader(const std::string &targetLib, const std::string &targetLibPath) :
        m_targetLib(getLibName(targetLib, targetLibPath)) {
    }

    ~TargetLoader() {
        if(m_releaseFunc) {
            m_releaseFunc(m_target);
        }

        if (m_imageHandle) {
            dlclose(m_imageHandle);
        }
    }

    Target* operator->() const noexcept {
        return m_target;
    }

    bool load() {
        m_imageHandle = dlopen(m_targetLib.c_str(), RTLD_LAZY);
        if (m_imageHandle == nullptr) {
            std::cerr << dlerror() << std::endl;
            return false;
        }

        auto *f_init = (Target::CreateFunc *) dlsym(m_imageHandle, "target_create");
        if (f_init == nullptr) {
            std::cerr << dlerror() << std::endl;
            return false;
        }

        m_releaseFunc = (Target::ReleaseFunc *) dlsym(m_imageHandle, "target_release");
        if (m_releaseFunc == nullptr) {
            std::cerr << dlerror() << std::endl;
            return false;
        }

        m_target = f_init("");
        return true;
    }

 private:
    Target *m_target;
    std::string m_targetLib;
    void *m_imageHandle;
    Target::ReleaseFunc *m_releaseFunc;

    const std::string getLibName(const std::string &libName, const std::string &modPath) const {
        std::stringstream name_builder;
        name_builder << modPath;
        if (modPath.length() > 0 && modPath[modPath.length() - 1] != '/') {
            name_builder << "/";
        }
        name_builder << "lib";
        name_builder << libName;
        name_builder << ".";
        name_builder << SCATTER_LIB_SUFFIX;

        return name_builder.str();
    }
};

}
}

#endif //SCATTER_TARGET_H
