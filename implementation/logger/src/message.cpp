// Copyright (C) 2020-2021 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>

#ifdef ANDROID
#include <android/log.h>
#define LOG_TAG "debug"
#define ALOGV(fmt, args...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, fmt, ##args)
#define ALOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#define ALOGI(fmt, args...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, fmt, ##args)
#define ALOGW(fmt, args...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, fmt, ##args)
#define ALOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##args)

#endif

#include <vsomeip/internal/logger.hpp>
#include <vsomeip/runtime.hpp>

#include "../include/logger_impl.hpp"
#include "../../configuration/include/configuration.hpp"

namespace vsomeip_v3 {
namespace logger {

std::mutex message::mutex__;

message::message(level_e _level)
    : std::ostream(&buffer_),
      level_(_level) {

    when_ = std::chrono::system_clock::now();
}

message::~message() try {
    std::lock_guard<std::mutex> its_lock(mutex__);
    auto its_logger = logger_impl::get();
    auto its_configuration = its_logger->get_configuration();

    if (!its_configuration)
        return;

    if (level_ > its_configuration->get_loglevel())
        return;

    if (its_configuration->has_console_log()
            || its_configuration->has_file_log()) {

        // Prepare log level
        const char *its_level;
        switch (level_) {
        case level_e::LL_FATAL:
            its_level = "fatal";
            break;
        case level_e::LL_ERROR:
            its_level = "error";
            break;
        case level_e::LL_WARNING:
            its_level = "warning";
            break;
        case level_e::LL_INFO:
            its_level = "info";
            break;
        case level_e::LL_DEBUG:
            its_level = "debug";
            break;
        case level_e::LL_VERBOSE:
            its_level = "verbose";
            break;
        default:
            its_level = "none";
        };

        // Prepare time stamp
        auto its_time_t = std::chrono::system_clock::to_time_t(when_);
        struct tm its_time;
#ifdef _WIN32
        localtime_s(&its_time, &its_time_t);
#else
        localtime_r(&its_time_t, &its_time);
#endif
        auto its_ms = (when_.time_since_epoch().count() / 100) % 1000000;

        if (its_configuration->has_console_log()) {
#ifndef ANDROID
            std::cout
                << std::dec
                << std::setw(4) << its_time.tm_year + 1900 << "-"
                << std::setfill('0')
                << std::setw(2) << its_time.tm_mon + 1 << "-"
                << std::setw(2) << its_time.tm_mday << " "
                << std::setw(2) << its_time.tm_hour << ":"
                << std::setw(2) << its_time.tm_min << ":"
                << std::setw(2) << its_time.tm_sec << "."
                << std::setw(6) << its_ms << " ["
                << its_level << "] "
                << buffer_.data_.str()
                << std::endl;
#else
            std::string app = runtime::get_property("LogApplication");

            switch (level_) {
            case level_e::LL_FATAL:
                ALOGE(app.c_str(), ("VSIP: " + buffer_.data_.str()).c_str());
                break;
            case level_e::LL_ERROR:
                ALOGE(app.c_str(), ("VSIP: " + buffer_.data_.str()).c_str());
                break;
            case level_e::LL_WARNING:
                ALOGW(app.c_str(), ("VSIP: " + buffer_.data_.str()).c_str());
                break;
            case level_e::LL_INFO:
                ALOGI(app.c_str(), ("VSIP: " + buffer_.data_.str()).c_str());
                break;
            case level_e::LL_DEBUG:
                ALOGD(app.c_str(), ("VSIP: " + buffer_.data_.str()).c_str());
                break;
            case level_e::LL_VERBOSE:
                ALOGV(app.c_str(), ("VSIP: " + buffer_.data_.str()).c_str());
                break;
            default:
                ALOGI(app.c_str(), ("VSIP: " + buffer_.data_.str()).c_str());
            };
#endif // !ANDROID
        }

        if (its_configuration->has_file_log()) {
            std::ofstream its_logfile(
                    its_configuration->get_logfile(),
                    std::ios_base::app);
            if (its_logfile.is_open()) {
                its_logfile
                    << std::dec
                    << std::setw(4) << its_time.tm_year + 1900 << "-"
                    << std::setfill('0')
                    << std::setw(2) << its_time.tm_mon + 1 << "-"
                    << std::setw(2) << its_time.tm_mday << " "
                    << std::setw(2) << its_time.tm_hour << ":"
                    << std::setw(2) << its_time.tm_min << ":"
                    << std::setw(2) << its_time.tm_sec << "."
                    << std::setw(6) << its_ms << " ["
                    << its_level << "] "
                    << buffer_.data_.str()
                    << std::endl;
            }
        }
    }
    if (its_configuration->has_dlt_log()) {
#ifdef USE_DLT
#ifndef ANDROID
        its_logger->log(level_, buffer_.data_.str().c_str());
#endif
#endif // USE_DLT
    }
} catch (const std::exception& e) {
    std::cout << "\nVSIP: Error destroying message class: " << e.what() << '\n';
}

std::streambuf::int_type
message::buffer::overflow(std::streambuf::int_type c) {
    if (c != EOF) {
        data_ << (char)c;
    }

    return c;
}

std::streamsize
message::buffer::xsputn(const char *s, std::streamsize n) {
    data_.write(s, n);
    return n;
}

} // namespace logger
} // namespace vsomeip_v3
