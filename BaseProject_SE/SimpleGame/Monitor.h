#pragma once
#include <string>

namespace Monitor
{
    std::string Quote(const std::string& value);
    void StartServer();
    void Publish(const std::string& body, bool focused);
}
