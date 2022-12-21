#ifndef IMAGE_PROCESSING_VIEW_H
#define IMAGE_PROCESSING_VIEW_H

#include <nanogui/nanogui.h>

#include <misc/image_processing.h>
#include <render/texture.h>

#include <gui/gui.h>

class DFTView final : public nanogui::ImageView {
public:
    DFTView(nanogui::Widget* parent, Toolbar* toolbar);

    void setImage(const Texture& image, bool fourierDisplay = false);

    void setNext(DFTView* view) { next = view; }

private:
    Texture image;
    DFTView* next{nullptr};
    nanogui::ref<nanogui::Texture> texture;
    nanogui::ref<nanogui::Widget> dftControls;
    bool fourierDisplay{false};
};

class SamplingView final : public nanogui::ImageView {
public:
    SamplingView(nanogui::Widget* parent, Toolbar* toolbar);

private:
    void updateImage();
    void perform_layout(NVGcontext* ctx) override
    {
        center();
        (void)ctx; // unused
    }

    Resolution res{512, 512};
    uint32_t numSamplesPerDim{4};
    Texture image{};
    nanogui::ref<nanogui::Texture> texture;
    nanogui::ref<nanogui::Widget> samplingControls;
    SamplingPatterns sampler;
};

#endif // IMAGE_PROCESSING_VIEW_H
