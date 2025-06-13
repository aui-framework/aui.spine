#pragma once

#include <AUI/Platform/AWindow.h>

class MainWindow: public AWindow {
public:
    MainWindow();

private:
    void loadFile(const APath& path);
};
