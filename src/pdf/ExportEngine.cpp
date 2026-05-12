#include "ExportEngine.h"
#include "Trace.h"
#include <QColor>
#include <mupdf/pdf.h>

bool ExportEngine::exportPdf(const QString &sourcePdf, 
                            const QString &targetPdf, 
                            const std::vector<Stroke> &strokes,
                            float verticalSpacing)
{
    fz_context *ctx = fz_new_context(nullptr, nullptr, FZ_STORE_DEFAULT);
    if (!ctx) return false;
    fz_register_document_handlers(ctx);

    pdf_document *doc = nullptr;
    pdf_document *out_doc = nullptr;

    fz_try(ctx) {
        doc = pdf_open_document(ctx, sourcePdf.toLocal8Bit().constData());
        out_doc = pdf_create_document(ctx);

        int count = pdf_count_pages(ctx, doc);
        float currentYOffset = 0;

        for (int i = 0; i < count; ++i) {
            fz_page *page = fz_load_page(ctx, (fz_document*)doc, i);
            fz_rect rect = fz_bound_page(ctx, page);
            float pageH = rect.y1 - rect.y0;
            
            fz_buffer *contents = fz_new_buffer(ctx, 1024);
            pdf_obj *resources = nullptr;
            fz_device *dev = nullptr;
            
            fz_try(ctx) {
                dev = pdf_page_write(ctx, out_doc, rect, &resources, &contents);
                
                // 1. Draw original page content
                fz_run_page(ctx, page, dev, fz_identity, nullptr);
                
                // 2. Draw strokes for this specific page
                for (const auto &s : strokes) {
                    if (s.points.size() < 2) continue;
                    
                    bool hasPointOnPage = false;
                    for (const auto &p : s.points) {
                        if (p.y >= currentYOffset && p.y <= currentYOffset + pageH) {
                            hasPointOnPage = true;
                            break;
                        }
                    }
                    if (!hasPointOnPage) continue;

                    fz_path *path = fz_new_path(ctx);
                    fz_moveto(ctx, path, s.points[0].x, s.points[0].y - currentYOffset);
                    for (size_t p = 1; p < s.points.size(); ++p) {
                        fz_lineto(ctx, path, s.points[p].x, s.points[p].y - currentYOffset);
                    }
                    
                    fz_stroke_state *ss = fz_new_stroke_state(ctx);
                    ss->linewidth = s.width;
                    
                    float color[3] = { (float)s.color.redF(), (float)s.color.greenF(), (float)s.color.blueF() };
                    fz_stroke_path(ctx, dev, path, ss, fz_identity, fz_device_rgb(ctx), color, s.color.alphaF(), fz_default_color_params);
                    
                    fz_drop_stroke_state(ctx, ss);
                    fz_drop_path(ctx, path);
                }
                
                fz_close_device(ctx, dev);
            }
            fz_always(ctx) {
                fz_drop_device(ctx, dev);
            }
            fz_catch(ctx) {
                fz_rethrow(ctx);
            }

            pdf_obj *page_obj = pdf_add_page(ctx, out_doc, rect, 0, resources, contents);
            pdf_insert_page(ctx, out_doc, -1, page_obj);
            
            fz_drop_buffer(ctx, contents);
            pdf_drop_obj(ctx, resources);
            fz_drop_page(ctx, page);
            
            currentYOffset += pageH + verticalSpacing;
        }
        
        fz_output *out = fz_new_output_with_path(ctx, targetPdf.toLocal8Bit().constData(), 0);
        fz_try(ctx) {
            pdf_write_document(ctx, out_doc, out, &pdf_default_write_options);
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
        if (out_doc) pdf_drop_document(ctx, out_doc);
        fz_drop_context(ctx);
        return false;
    }

    if (doc) pdf_drop_document(ctx, doc);
    if (out_doc) pdf_drop_document(ctx, out_doc);
    fz_drop_context(ctx);
    return true;
}
