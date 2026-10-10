//******************************************************************************
// Copyright (c) 2023 Autodesk Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef __RvCommon__FileLogger__h__
#define __RvCommon__FileLogger__h__

#include <spdlog/logger.h>
#include <string>

namespace Rv
{

    class FileLogger
    {
    public:
        FileLogger();
        ~FileLogger();

        void logToFile(spdlog::level::level_enum lineLevel, const std::string& line);

    private:
        spdlog::logger* m_logger;

        void setLogLevel(const std::string& level);
    };

} // namespace Rv

#endif
