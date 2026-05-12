#include "PdfEngine.h"
#include <QDebug>

PdfEngine::PdfEngine()
{
    initContext();
}

PdfEngine::~PdfEngine()
{
    closeDocument();
    if (m_ctx) {
        fz_drop_context(m_ctx);
    }
}

#include <QMutex>
#include <vector>

static QMutex g_mupdf_mutexes[FZ_LOCK_MAX];

static void lock_mupdf(void *user, int lock) {
    g_mupdf_mutexes[lock].lock();
}

static void unlock_mupdf(void *user, int lock) {
    g_mupdf_mutexes[lock].unlock();
}

static fz_locks_context g_mupdf_locks = {
    nullptr,
    lock_mupdf,
    unlock_mupdf
};

void PdfEngine::initContext()
{
    printf("PdfEngine: Initializing context with locks...\n");
    m_ctx = fz_new_context(nullptr, &g_mupdf_locks, FZ_STORE_DEFAULT);
    if (!m_ctx) {
        printf("PdfEngine: FAILED to initialize context\n");
        return;
    }
    fz_register_document_handlers(m_ctx);
    printf("PdfEngine: Context initialized: %p\n", (void*)m_ctx);
}

bool PdfEngine::loadDocument(const std::string& path)
{
    QMutexLocker locker(&m_mutex);
    closeDocument();

    if (!m_ctx) return false;

    printf("PdfEngine: Opening document: %s\n", path.c_str());
    fz_try(m_ctx) {
        m_doc = fz_open_document(m_ctx, path.c_str());
        printf("PdfEngine: Document opened: %p\n", (void*)m_doc);
    }
    fz_catch(m_ctx) {
        printf("PdfEngine: FAILED to open document\n");
        return false;
    }

    return true;
}

void PdfEngine::closeDocument()
{
    if (m_doc) {
        fz_drop_document(m_ctx, m_doc);
        m_doc = nullptr;
    }
}

int PdfEngine::pageCount() const
{
    QMutexLocker locker(&m_mutex);
    if (!m_ctx || !m_doc) return 0;

    int count = 0;
    fz_try(m_ctx) {
        count = fz_count_pages(m_ctx, m_doc);
    }
    fz_catch(m_ctx) {
        return 0;
    }
    return count;
}

QSizeF PdfEngine::pageSize(int pageNum) const
{
    QMutexLocker locker(&m_mutex);
    if (!m_ctx || !m_doc) return QSizeF();

    fz_page* page = nullptr;
    fz_rect rect;
    fz_try(m_ctx) {
        page = fz_load_page(m_ctx, m_doc, pageNum);
        rect = fz_bound_page(m_ctx, page);
        fz_drop_page(m_ctx, page);
    }
    fz_catch(m_ctx) {
        return QSizeF();
    }
    return QSizeF(rect.x1 - rect.x0, rect.y1 - rect.y0);
}

QImage PdfEngine::renderTile(int pageNum, int x, int y, int width, int height, float scale)
{
    QMutexLocker locker(&m_mutex);
    if (!m_ctx) {
        qWarning() << "PdfEngine: No context";
        return QImage();
    }
    if (!m_doc) {
        qWarning() << "PdfEngine: No document loaded";
        return QImage();
    }

    int count = fz_count_pages(m_ctx, m_doc);
    if (pageNum < 0 || pageNum >= count) {
        qWarning() << "PdfEngine: Invalid page number" << pageNum;
        return QImage();
    }

    if (width <= 0 || height <= 0) {
        return QImage();
    }

    fz_pixmap* pix = nullptr;
    fz_page* page = nullptr;
    QImage image;

    fz_try(m_ctx) {
        page = fz_load_page(m_ctx, m_doc, pageNum);
        
        fz_matrix ctm = fz_scale(scale, scale);
        fz_irect bbox = fz_make_irect(x, y, x + width, y + height);
        
        pix = fz_new_pixmap_with_bbox(m_ctx, fz_device_rgb(m_ctx), bbox, nullptr, 0);
        if (pix) {
            fz_clear_pixmap_with_value(m_ctx, pix, 255);
            
            fz_device* dev = fz_new_draw_device(m_ctx, ctm, pix);
            fz_run_page(m_ctx, page, dev, ctm, nullptr);
            fz_close_device(m_ctx, dev);
            fz_drop_device(m_ctx, dev);
            
            unsigned char* samples = fz_pixmap_samples(m_ctx, pix);
            int w = fz_pixmap_width(m_ctx, pix);
            int h = fz_pixmap_height(m_ctx, pix);
            int n = fz_pixmap_components(m_ctx, pix);
            
            if (n == 3) {
                image = QImage(samples, w, h, w * 3, QImage::Format_RGB888).copy();
            } else if (n == 4) {
                image = QImage(samples, w, h, w * 4, QImage::Format_RGBA8888).copy();
            }
            
            fz_drop_pixmap(m_ctx, pix);
        }
        fz_drop_page(m_ctx, page);
    }
    fz_catch(m_ctx) {
        qWarning() << "PdfEngine: MuPDF error during rendering";
        if (pix) fz_drop_pixmap(m_ctx, pix);
        if (page) fz_drop_page(m_ctx, page);
        return QImage();
    }

    return image;
}
