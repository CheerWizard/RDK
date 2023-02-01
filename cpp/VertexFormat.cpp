#include <VertexFormat.h>

namespace rdk {

    void VertexFormat::add(VertexAttr &&attr) {
        m_Attrs.emplace_back(attr);
    }

    void VertexFormat::remove(const VertexAttr &attr) {
        m_Attrs.erase(std::find(m_Attrs.begin(), m_Attrs.end(), attr));
    }

    void VertexFormat::clear() {
        m_Attrs.clear();
    }

    bool VertexFormat::exists(const VertexAttr &attr) {
        return std::find(m_Attrs.begin(), m_Attrs.end(), attr) != m_Attrs.end();
    }

}