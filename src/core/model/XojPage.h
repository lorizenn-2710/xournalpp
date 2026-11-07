/*
 * Xournal++
 *
 * A page (PDF or drawings or both)
 *
 * @author Xournal++ Team
 * https://github.com/xournalpp/xournalpp
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <cstddef>   // for size_t
#include <optional>  // for optional
#include <string>    // for string
#include <vector>    // for vector

#include "util/Color.h"  // for Color
#include "util/PointerContainerView.h"
#include "util/Util.h"  // for npos

#include "BackgroundImage.h"  // for BackgroundImage
#include "Layer.h"            // for Layer, Layer::Index
#include "PageHandler.h"      // for PageHandler
#include "PageType.h"         // for PageType

// Document-owned vector paper; view geometry never changes element coordinates.
struct FloatingNoteState {
    std::string name, sourceId;
    double x = 24, y = 24, width = 440, height = 430;
    double zoom = .75, scrollX = 0, scrollY = 0;
    bool open = true, pinned = false, deleted = false;
    size_t created = 0, modified = 0, revision = 0, zOrder = 0;
};

class XojPage: public PageHandler {
public:
    XojPage(double width, double height, bool suppressLayerCreation = false);
    ~XojPage() override;
    XojPage(const XojPage& page);
    void operator=(const XojPage& p) = delete;

    std::string stableId;
    std::optional<FloatingNoteState> floatingNote;

    // Do not modify layers directly, use LayerController
    // So notification can be sent on change
protected:
    void addLayer(Layer* layer);
    void insertLayer(Layer* layer, Layer::Index index);
    void removeLayer(Layer* layer);
    void setLayerVisible(Layer::Index layerId, bool visible);

public:
    // Also set the size over doc->setPageSize!
    void setBackgroundPdfPageNr(size_t page);

    void setBackgroundType(const PageType& bgType);
    PageType getBackgroundType() const;

    /**
     * Do not call this, cal doc->setPageSize(Page * p, double width, double height);
     */
    void setSize(double width, double height);

    double getWidth() const;
    double getHeight() const;

    size_t getPdfPageNr() const;

    bool isAnnotated() const;

    void setBackgroundColor(Color color);
    Color getBackgroundColor() const;

    std::vector<Layer*>& getLayers();
    xoj::util::PointerContainerView<std::vector<Layer*>> getLayersView() const;

    Layer::Index getLayerCount() const;
    Layer::Index getSelectedLayerId();
    void setSelectedLayerId(Layer::Index id);
    bool isLayerVisible(Layer::Index layerId) const;

    Layer* getSelectedLayer();

    BackgroundImage& getBackgroundImage();
    const BackgroundImage& getBackgroundImage() const;
    void setBackgroundImage(BackgroundImage img);

    std::string getBackgroundName() const;
    bool backgroundHasName() const;
    void setBackgroundName(const std::string& newName);

    /**
     * Copies this page an all it's contents to a new page
     */
    XojPage* clone();

private:
    /**
     * The Background image if any
     */
    BackgroundImage backgroundImage;

    /**
     * The size of the page
     */
    double width = 0;
    double height = 0;

    /**
     * The layer list
     */
    std::vector<Layer*> layer;

    /**
     * The current selected layer ID
     */
    Layer::Index currentLayer = npos;

    /**
     * The Background Type of the page
     */
    PageType bgType;

    /**
     * If the page has a PDF background, the page number of the PDF Page
     */
    size_t pdfBackgroundPage = npos;

    /**
     * The background color if the background type is plain
     */
    Color backgroundColor{Colors::white};

    /**
     * Background visible
     */
    bool backgroundVisible = true;

    /**
     * Background name
     */
    std::optional<std::string> backgroundName;

    // Allow LoadHandler to add layers directly
    friend class LoadHandler;

    // Allow LayerController to modify layers of a page
    // Notifications were be sent
    friend class LayerController;
};
