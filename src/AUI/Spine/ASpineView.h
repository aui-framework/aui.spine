#pragma once

#include <spine/spine.h>
#include <AUI/View/AView.h>
#include <AUI/Util/APimpl.h>

class ASpineView : public AView {
public:
    static struct TextureLoader : spine::TextureLoader {
        ~TextureLoader() override = default;
        void load(spine::AtlasPage& page, const spine::String& path) override;
        void unload(void* texture) override;
    } TEXTURE_LOADER;

    enum class Sizing {
        /**
         * @brief Scale the skeleton to fit the view.
         */
        UNIFORM,

        /**
         * @brief Do not apply any scaling.
         */
        AS_IS,
    };

    ASpineView(
        _<spine::Atlas> atlas, _<spine::SkeletonData> skeletonData, _<spine::AnimationStateData> animationStateData);
    ~ASpineView() override;

    void update(float delta, spine::Physics physics) {
        mAnimationState.update(delta);
        mAnimationState.apply(mSkeleton);
        mSkeleton.update(delta);
        mSkeleton.updateWorldTransform(physics);
    }

    [[nodiscard]]
    _<spine::Atlas>& atlas() { return mAtlas; }

    [[nodiscard]]
    _<spine::SkeletonData>& skeletonData() { return mSkeletonData; }

    [[nodiscard]]
    _<spine::AnimationStateData>& animationStateData() { return mAnimationStateData; }

    [[nodiscard]]
    spine::Skeleton& skeleton() { return mSkeleton; }

    [[nodiscard]]
    spine::AnimationState& animationState() { return mAnimationState; }

    void setUsePma(bool usePma) { mUsePma = usePma; }
    void setSizing(Sizing sizing) { mSizing = sizing; }

    void render(ARenderContext ctx) override;

private:
    _<spine::Atlas> mAtlas;
    _<spine::SkeletonData> mSkeletonData;
    _<spine::AnimationStateData> mAnimationStateData;
    Sizing mSizing = Sizing::UNIFORM;
    spine::Skeleton mSkeleton;
    spine::AnimationState mAnimationState;
    AOptional<spine::SkeletonRenderer> mSkeletonRenderer;
    bool mUsePma = false;
    struct Impl;
    aui::fast_pimpl<Impl, 128> mImpl;
    std::chrono::high_resolution_clock::time_point mStartTime = std::chrono::high_resolution_clock::now();
};
