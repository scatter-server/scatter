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
#include <dlfcn.h>
#include <sys/stat.h>
#include <json.hpp>
#include "Message.h"

namespace wss {
namespace event {

class TargetLoader;

/// \brief Available:
/// postback: PostbackTarget
///     "url": "http://example.com/postback",
class Target {
 public:
    friend class wss::event::TargetLoader;

    // DEFS
    typedef Target *(CreateFunc)(const nlohmann::json &);
    typedef void    (ReleaseFunc)(Target *);

    /// \brief Accept json config of entire target object
    /// \param config
    explicit Target(const nlohmann::json &config) :
        m_config(config),
        m_validState(true),
        m_errorMessage("") {
    }

    virtual ~Target() = default;

    /// \brief Send event to entire target
    /// \param payload Payload to send
    /// \param error if method returned false, error will contains error message
    /// \return true if sending complete
    virtual bool send(const wss::MessagePayload &payload, std::string &error) = 0;
    virtual std::string getType() = 0;

    virtual /// \brief Check target is in valid state
    /// \return valid state of target object
    bool isValid() const {
        return m_validState;
    }

    virtual /// \brief If target is not valid, message will be available
    /// \return error message
    std::string getErrorMessage() const {
        return m_errorMessage;
    }

    virtual void addFallback(std::shared_ptr<wss::event::Target> &&fallbackTarget) {
        fallbackTargets.push_back(fallbackTarget);
    }

    virtual const std::vector<std::shared_ptr<wss::event::Target>> getFallbacks() const {
        return fallbackTargets;
    }

 protected:
    virtual /// \brief Set error message to read when object is invalid state. Mark object as in invalid state/
    /// \param msg
    void setErrorMessage(const std::string &msg) {
        m_validState = false;
        m_errorMessage = msg;
    }

    virtual /// \brief Set error message to read when object is invalid state. Mark object as in invalid state/
    /// \param msg rvalue string
    void setErrorMessage(std::string &&msg) {
        m_validState = false;
        m_errorMessage = std::move(msg);
    }

    virtual /// \brief Appends to existing message new message separated by new line. If message did not set before, it will be a single message.
    /// Mark object as in invalid state/
    /// \param msg rvalue error message
    void appendErrorMessage(std::string &&msg) {
        m_validState = false;
        if (m_errorMessage.empty()) {
            setErrorMessage(std::move(msg));
            return;
        }

        m_errorMessage = fmt::format("{0}\n{1}", m_errorMessage, msg);
    }

    virtual /// \brief Appends to existing message new message separated by new line. If message did not set before, it will be a single message.
    /// Mark object as in invalid state/
    /// \param msg lvalue error message
    void appendErrorMessage(const std::string &msg) {
        m_validState = false;
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
class TargetLoader final : public Target {
 public:
    TargetLoader(const nlohmann::json &config, const std::string &targetLib) :
        Target(config),
        m_target(nullptr),
        m_config(config),
        m_targetLib(targetLib),
        m_imageHandle(nullptr),
        m_releaseFunc(nullptr) { }

    ~TargetLoader() override {
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

    bool send(const wss::MessagePayload &payload, std::string &error) override {
        return m_target->send(payload, error);
    }
    std::string getType() override {
        return m_target->getType();
    }

    bool isValid() const override {
        return m_target->isValid();
    }
    std::string getErrorMessage() const override {
        return m_target->getErrorMessage();
    }
    void addFallback(std::shared_ptr<wss::event::Target> &&fallbackTarget) override {
        m_target->addFallback(std::move(fallbackTarget));
    }
    const std::vector<std::shared_ptr<wss::event::Target>> getFallbacks() const override {
        return m_target->getFallbacks();
    }

    bool load(const std::vector<std::string> &searchPaths, std::string &loadErrorRef) {
        std::string pathToLoad;
        for (auto &searchPath: searchPaths) {
            const std::string libName = getLibName(m_targetLib, searchPath);
            if (fileExists(libName)) {
                pathToLoad = libName;
                break;
            }
        }

        if (pathToLoad.empty()) {
            std::stringstream ss;
            ss << "Extension library " << m_targetLib << " didn't found in paths: " << std::endl;
            for (const auto &p: searchPaths) {
                ss << "\t - " << getLibName(m_targetLib, p) << std::endl;
            }
            loadErrorRef = ss.str();
            return false;
        }

        m_imageHandle = dlopen(pathToLoad.c_str(), RTLD_LAZY);
        if (m_imageHandle == nullptr) {
            loadErrorRef = dlerror();
            return false;
        }

        auto *f_init = (Target::CreateFunc *) dlsym(m_imageHandle, "target_create");
        if (f_init == nullptr) {
            loadErrorRef = dlerror();
            return false;
        }

        m_releaseFunc = (Target::ReleaseFunc *) dlsym(m_imageHandle, "target_release");
        if (m_releaseFunc == nullptr) {
            loadErrorRef = dlerror();
            return false;
        }

        try {
            m_target = f_init(m_config);
        } catch (const std::exception &e) {
            loadErrorRef = e.what();
            return false;
        }

        return true;
    }

 protected:
    void setErrorMessage(const std::string &msg) override {
        m_target->setErrorMessage(msg);
    }
    void setErrorMessage(std::string &&msg) override {
        m_target->setErrorMessage(std::move(msg));
    }
    void appendErrorMessage(std::string &&msg) override {
        m_target->appendErrorMessage(std::move(msg));
    }
    void appendErrorMessage(const std::string &msg) override {
        m_target->appendErrorMessage(msg);
    }

 private:
    Target *m_target;
    nlohmann::json m_config;
    std::string m_targetLib;
    void *m_imageHandle;
    Target::ReleaseFunc *m_releaseFunc;

    bool fileExists(const std::string &path) {
        struct stat s;
        return stat(path.c_str(), &s) == 0;
    }

    const std::string getLibName(const std::string &libName, const std::string &modPath) const {
        std::stringstream name_builder;
        name_builder << modPath;
        if (modPath.length() > 0 && modPath[modPath.length() - 1] != '/') {
            name_builder << "/";
        }
        name_builder << "libscatter_event_";
        name_builder << libName;
        name_builder << ".";
        name_builder << SCATTER_LIB_SUFFIX;

        return name_builder.str();
    }
};

}
}

#endif //SCATTER_TARGET_H
