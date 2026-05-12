#pragma once

#include <string>
#include <QImage>
#include <QMutex>

#ifdef ENABLE_PDF
#include <mupdf/fitz.h>
#endif

class PdfEngine {
public:
    PdfEngine();
    ~PdfEngine();

    bool loadDocument(const std::string& path);
    void closeDocument();

    int pageCount() const;
    QSizeF pageSize(int pageNum) const;

    // Tiled rendering: renders a specific portion of the page
    QImage renderTile(int pageNum, int x, int y, int width, int height, float scale);

private:
#ifdef ENABLE_PDF
    fz_context* m_ctx = nullptr;
    fz_document* m_doc = nullptr;
#endif
    mutable QMutex m_mutex;

    void initContext();
};
