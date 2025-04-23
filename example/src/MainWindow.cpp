#include <range/v3/all.hpp>
#include "MainWindow.h"
#include <AUI/Spine/ASpineView.h>
#include <AUI/Util/UIBuildingHelpers.h>
#include <AUI/View/ALabel.h>
#include <AUI/View/AButton.h>
#include <AUI/Platform/APlatform.h>
#include <AUI/View/ADrawableView.h>
#include <AUI/View/AProgressBar.h>
#include <AUI/View/AForEachUI.h>
#include <AUI/View/AGroupBox.h>
#include <AUI/View/ASlider.h>
#include <AUI/View/ARadioGroup.h>
#include <AUI/View/ACheckBox.h>

using namespace declarative;

template <typename T>
std::span<const T> asSpan(spine::Vector<T>& v) {
    return { v.buffer(), v.size() };
}

MainWindow::MainWindow() : AWindow("spine-aui", 650_dp, 600_dp) {
    // we use a y-down coordinate system.
    spine::Bone::setYDown(true);

    auto atlas = [] {
        auto buffer = AByteBuffer::fromStream(":anim/spineboy.atlas"_url.open());
        return _new<spine::Atlas>(buffer.data(), buffer.size(), ":anim", &ASpineView::TEXTURE_LOADER);
    }();
    static spine::SkeletonBinary binary(atlas.get());
    auto skeletonData = [&] {
        auto buffer = AByteBuffer::fromStream(":anim/spineboy-pro.skel"_url.open());
        return aui::ptr::manage(
            binary.readSkeletonData(reinterpret_cast<const unsigned char*>(buffer.data()), buffer.size()));
    }();
    if (!binary.getError().isEmpty()) {
        throw AException("SkeletonBinary failed: {}"_format(binary.getError().buffer()));
    }

    auto animationStateData = _new<spine::AnimationStateData>(skeletonData.get());
    auto animations = asSpan(animationStateData->getSkeletonData()->getAnimations());
    animationStateData->setDefaultMix(0.2f);

    auto spineView = _new<ASpineView>(atlas, skeletonData, animationStateData) with_style { Expanding() } let {
        // Set the skeleton's position to the center of
        // the screen and scale it to make it smaller.
        it->skeleton().setPosition(0, 0);
        it->skeleton().setScaleX(0.5);
        it->skeleton().setScaleY(0.5);

        // Create an AnimationState to drive animations on the skeleton. Set the "portal" animation
        // on track with index 0.
        it->animationState().setAnimation(0, "portal", true);
        it->animationState().addAnimation(0, "run", true, 0);
    };

    auto backgroundLayer = _new<AView>() with_style { Expanding() };
    setContents(Stacked {
      backgroundLayer,
      spineView,
      Label { "+" } with_style { FontRendering::NEAREST }, // anchor point 0,0
      Horizontal::Expanding {
        Vertical {
          GroupBox {
            Label { "setAnimation" },
            AUI_DECLARATIVE_FOR(i, animations, AVerticalLayout) {
              auto a = const_cast<spine::Animation*>(i);
              return Button { a->getName().buffer() }.clicked(this, [=] {
                spineView->animationState().setAnimation(0, a, true);
              });
            },
          },
          GroupBox {
            Label { "Mixing" },
            Vertical {
              _new<ASlider>() let {
                  it->value() = 0.2f;
                  connect(it->value(), [animationStateData](float v) { animationStateData->setDefaultMix(v); });
                },
              Horizontal {
                Label { "0.0" },
                SpacerExpanding(),
                Label { "1.0" },
              },
            },
          },
          CheckBoxWrapper { Label { "PMA rendering" } } with_style { Padding(8_dp) } let {
              connect(it->checked(), slot(spineView)::setUsePma);
          },
          GroupBox {
            Label { "Background" },
            RadioGroup {
              RadioButton { "white" },
              RadioButton { "black" },
              RadioButton { "art" },
              RadioButton { "colorful" },
            } let {
                it->selectionId() = 0;
                connect(it->selectionId(), [backgroundLayer](int i) {
                     switch (i) {
                         case 0:
                             backgroundLayer->setCustomStyle({ BackgroundSolid(AColor::WHITE), Expanding(), });
                             break;
                         case 1:
                             backgroundLayer->setCustomStyle({ BackgroundSolid(AColor::BLACK), Expanding(), });
                             break;
                         default:
                             backgroundLayer->setCustomStyle({ BackgroundImage(":bg/{}.jpg"_format(i - 1), {}, {}, Sizing::CONTAIN), Expanding(), });
                             break;
                     }
                });
            }
          },
        } with_style {
          MinSize { 150_dp },
          Backdrop { Backdrop::GaussianBlur { 30_dp } },
          BackgroundSolid { AColor::WHITE.transparentize(0.5f) },
        },
      },
    } with_style { Padding(0) });
}
