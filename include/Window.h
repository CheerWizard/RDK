#pragma once

#include <Core.h>
#include <vector>

namespace rdk {

    struct Resizable final {
        static u8 FALSE;
        static u8 TRUE;
    };

    class WindowListener {
    public:
        virtual void onFrameBufferResized(int width, int height) = 0;
    };

    class Window final {

    public:
        Window(const char* title, int width, int height, WindowListener* listener);
        ~Window();

    public:
        void update();
        void setResizable(u8 resizable);
        bool shouldClose();

        inline const std::vector<const char*>& getExtensions() {
            return m_Extensions;
        }

        void addExtension(const char* && extension);

        inline void* getHandle() {
            return m_Handle;
        }

    private:
        void* m_Handle;
        const char* m_Title;
        int m_Width, m_Height;
        std::vector<const char*> m_Extensions;
    };

}