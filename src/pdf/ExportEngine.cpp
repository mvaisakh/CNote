#include "ExportEngine.h"
#include "Trace.h"
#include <QColor>
#include <mupdf/pdf.h>

bool ExportEngine::exportPdf(const QString &sourcePdf, 
                            const QString &targetPdf, 
                            const std::vector<Stroke> &strokes,
                            float verticalSpacing,
                            const QSizeF &canvasSize)
{
    fz_context *ctx = fz_new_context(nullptr, nullptr, FZ_STORE_DEFAULT);
    if (!ctx) return false;
    fz_register_document_handlers(ctx);

    pdf_document *doc = nullptr;

    fz_try(ctx) {
        doc = pdf_open_document(ctx, sourcePdf.toLocal8Bit().constData());
        int count = pdf_count_pages(ctx, doc);
        float currentYOffset = 0;

        for (int i = 0; i < count; ++i) {
            pdf_page *page = pdf_load_page(ctx, doc, i);
            fz_rect rect = pdf_bound_page(ctx, page, FZ_CROP_BOX);
            float pageW = rect.x1 - rect.x0;
            float pageH = rect.y1 - rect.y0;
            
            // Recalculate scaling and offset used in NoteCanvas using REAL canvas size
            float baseScale = std::min((float)canvasSize.width() / pageW, 
                                       (float)canvasSize.height() / pageH);
            int targetW = pageW * baseScale;
            int targetH = pageH * baseScale;
            int offsetX = (canvasSize.width() - targetW) / 2;

            for (const auto &s : strokes) {
                if (s.points.size() < 2) continue;
                
                bool onPage = false;
                for (const auto &p : s.points) {
                    if (p.y >= currentYOffset && p.y <= currentYOffset + targetH) {
                        onPage = true;
                        break;
                    }
                }
                if (!onPage) continue;

                pdf_annot *annot = pdf_create_annot(ctx, page, PDF_ANNOT_INK);
                
                int n = s.points.size();
                fz_point *pts = (fz_point*)fz_malloc(ctx, sizeof(fz_point) * n);
                for (int j = 0; j < n; ++j) {
                    // 1. Map Canvas Space -> PDF Local Space (Scaled)
                    float lx = s.points[j].x - offsetX;
                    float ly = s.points[j].y - currentYOffset;

                    // 2. Map PDF Local Space (Scaled) -> PDF Native Space (Points)
                    pts[j].x = (lx / baseScale) + rect.x0;
                    pts[j].y = rect.y1 - (ly / baseScale);
                }
                
                pdf_set_annot_ink_list(ctx, annot, 1, &n, pts);
                fz_free(ctx, pts);
                
                float color[3] = { (float)s.color.redF(), (float)s.color.greenF(), (float)s.color.blueF() };
                pdf_set_annot_color(ctx, annot, 3, color);
                pdf_set_annot_opacity(ctx, annot, s.color.alphaF());
                pdf_set_annot_border(ctx, annot, s.width / baseScale);
                
                pdf_update_annot(ctx, annot);
            }
            
            fz_drop_page(ctx, (fz_page*)page);
            currentYOffset += targetH + verticalSpacing;
        }
        
        fz_output *out = fz_new_output_with_path(ctx, targetPdf.toLocal8Bit().constData(), 0);
        fz_try(ctx) {
            pdf_write_document(ctx, doc, out, &pdf_default_write_options);
            fz_close_output(ctx, out);
        }
        fz_always(ctx) {
            fz_drop_output(ctx, out);
        }
        fz_catch(ctx) {
            fz_rethrow(ctx);
        }
    }
    fz_catch(ctx) {
        CN_TRACE("Export failed: %s", fz_caught_message(ctx));
        if (doc) pdf_drop_document(ctx, doc);
        fz_drop_context(ctx);
        return false;
    }

    if (doc) pdf_drop_document(ctx, doc);
    fz_drop_context(ctx);
    return true;
}
