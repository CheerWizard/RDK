#pragma once

#include <Core.h>
#include <vector>


namespace rdk {

    struct VertexBindDescriptor final {
        u32 binding = 0;
        u32 stride = 0;
        int inputRate = 0;

        VertexBindDescriptor() = default;
    };

    struct VertexAttr final {
        u32 binding = 0;
        u32 location = 0;
        int format = 0;
        u32 offset = 0;

        VertexAttr() = default;

        inline bool operator ==(const VertexAttr& other) const {
            return location == other.location && binding == other.binding && format == other.format && offset == other.offset;
        }
    };

    class VertexFormat final {

    public:
        VertexFormat() = default;
        VertexFormat(const VertexBindDescriptor& bindDescriptor, const std::initializer_list<VertexAttr>& attrs)
        : m_BindDescriptor(bindDescriptor), m_Attrs(attrs) {}

    public:
        void add(VertexAttr&& attr);
        void remove(const VertexAttr& attr);
        void clear();
        bool exists(const VertexAttr& attr);

        inline std::vector<VertexAttr>& getAttrs() {
            return m_Attrs;
        }

        inline VertexBindDescriptor& getDescriptor() {
            return m_BindDescriptor;
        }

        inline void setDescriptor(const VertexBindDescriptor& bindDescriptor) {
            m_BindDescriptor = bindDescriptor;
        }

    private:
        VertexBindDescriptor m_BindDescriptor;
        std::vector<VertexAttr> m_Attrs;
    };

}
