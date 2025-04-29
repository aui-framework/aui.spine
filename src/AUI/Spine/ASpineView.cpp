#include "ASpineView.h"
#include "AUISL/Generated/spine.vsh.glsl120.h"
#include "AUISL/Generated/spine.fsh.glsl120.h"
#include <AUI/Render/IRenderer.h>
#include <AUI/Platform/AWindow.h>
#include <AUI/GL/Vao.h>
#include <AUI/GL/Vbo.h>
#include <AUI/GL/Program.h>
#include <AUI/GL/ShaderUniforms.h>

ASpineView::TextureLoader ASpineView::TEXTURE_LOADER{};

static constexpr auto LOG_TAG = "ASpineView";

namespace {
struct Vertex {
    glm::vec2 pos;
    glm::vec2 uv;
    uint32_t color;
    uint32_t darkColor;
};
/**
 * @brief Blend mode expressed as OpenGL blend functions.
 * @details
 * See https://en.esotericsoftware.com/spine-slots#Blending
 */
struct BlendMode {
    unsigned int source_color;
    unsigned int source_color_pma;
    unsigned int dest_color;
    unsigned int source_alpha;
};

static constexpr BlendMode BLEND_MODES[] = {
    {
      .source_color = (unsigned int) GL_SRC_ALPHA,
      .source_color_pma = (unsigned int) GL_ONE,
      .dest_color = (unsigned int) GL_ONE_MINUS_SRC_ALPHA,
      .source_alpha = (unsigned int) GL_ONE,
    },
    {
      .source_color = (unsigned int) GL_SRC_ALPHA,
      .source_color_pma = (unsigned int) GL_ONE,
      .dest_color = (unsigned int) GL_ONE,
      .source_alpha = (unsigned int) GL_ONE,
    },
    {
      .source_color = (unsigned int) GL_DST_COLOR,
      .source_color_pma = (unsigned int) GL_DST_COLOR,
      .dest_color = (unsigned int) GL_ONE_MINUS_SRC_ALPHA,
      .source_alpha = (unsigned int) GL_ONE_MINUS_SRC_ALPHA,
    },
    {
      .source_color = (unsigned int) GL_ONE,
      .source_color_pma = (unsigned int) GL_ONE,
      .dest_color = (unsigned int) GL_ONE_MINUS_SRC_COLOR,
      .source_alpha = (unsigned int) GL_ONE_MINUS_SRC_COLOR,
    },
};

template<typename Vertex, typename Fragment>
inline void useAuislShader(AOptional<gl::Program>& out) {
    out.emplace();
    out->loadRaw(Vertex::code(), Fragment::code());
    Vertex::setup(out->handle());
    Fragment::setup(out->handle());
    out->compile();
}

static gl::Program& shader() {
    static AOptional<gl::Program> program;
    do_once {
        useAuislShader<aui::sl_gen::spine::vsh::glsl120::Shader, aui::sl_gen::spine::fsh::glsl120::Shader>(program);
    };
    return *program;
}

}   // namespace

struct ASpineView::Impl {
    gl::Vao vao;
    gl::VertexBuffer vertices;
    AVector<Vertex> cpuVertexBuffer;
};

ASpineView::ASpineView(_<spine::Atlas> atlas, _<spine::SkeletonData> skeletonData, _<spine::AnimationStateData> animationStateData)
  : mAtlas(std::move(atlas))
  , mSkeletonData(std::move(skeletonData))
  , mAnimationStateData(std::move(animationStateData))
  , mSkeleton(mSkeletonData.get())
  , mAnimationState(mAnimationStateData.get()) {
    mImpl->vao.bind();
    mImpl->vertices.bind();

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, uv));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void *) offsetof(Vertex, color));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void *) offsetof(Vertex, darkColor));
    glEnableVertexAttribArray(3);

    gl::Vao::unbind();
}

ASpineView::~ASpineView() = default;

constexpr uint32_t remapColor(uint32_t color) {
    return (color & 0xFF00FF00) | ((color & 0x00FF0000) >> 16) | ((color & 0x000000FF) << 16);
}

void ASpineView::render(ARenderContext ctx) {
    AView::render(ctx);

    if (!mSkeletonRenderer) {
        mSkeletonRenderer.emplace();
    }

    mImpl->vao.bind();
    shader().use();

    const auto now = std::chrono::high_resolution_clock::now();
    float delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - mStartTime).count() / 1000.f;
//    ALOG_DEBUG(LOG_TAG) << "Delta: " << delta;
    mStartTime = now;

    // Update and apply the animation state to the skeleton
    mAnimationState.update(delta);
    mAnimationState.apply(mSkeleton);

    // Update the skeleton time (used for physics)
    mSkeleton.update(delta);

    // Calculate the new pose
    mSkeleton.updateWorldTransform();

    for (auto command = mSkeletonRenderer->render(mSkeleton); command != nullptr; command = command->next) {
        mImpl->cpuVertexBuffer.clear(); // performance hint: won't release the memory.
        for (int i = 0; i < command->numVertices; ++i) {
            mImpl->cpuVertexBuffer.push_back(Vertex {
              .pos = { command->positions[i * 2], command->positions[i * 2 + 1] },
              .uv = { command->uvs[i * 2], command->uvs[i * 2 + 1] },
              .color = remapColor(command->colors[i]),
              .darkColor = remapColor(command->darkColors[i]),
            });
        }
        mImpl->vertices.set(mImpl->cpuVertexBuffer);

        mImpl->vao.indices(AArrayView<uint16_t>(command->indices, command->numIndices));

        const auto blendMode = BLEND_MODES[command->blendMode];
        glBlendFuncSeparate(mUsePma ? (GLenum) blendMode.source_color_pma : (GLenum) blendMode.source_color, (GLenum) blendMode.dest_color, (GLenum) blendMode.source_alpha, (GLenum) blendMode.dest_color);
        static_cast<gl::Texture2D*>(command->texture)->bind();
        auto mat = ctx.render.getTransform();
        mat = glm::translate(mat, glm::vec3(*size() / 2, 0.f));

        if (mSizing == Sizing::UNIFORM) {
            float scale = glm::min(size()->x / mSkeletonData->getWidth(), size()->y / mSkeletonData->getHeight()) * 0.5;
            mat = glm::scale(mat, glm::vec3(scale, scale, 1.f));
        }

        shader().set(aui::ShaderUniforms::TRANSFORM, mat);
        mImpl->vao.drawElements();
    }
    ctx.render.setBlending(Blending::NORMAL);
    gl::Vao::unbind();
    redraw();
}

static GLint parseFilter(spine::TextureFilter filter) {
    switch (filter) {
        case spine::TextureFilter_Nearest:
            return GL_NEAREST;

        default:
            return GL_LINEAR;
    }
}

static GLint parseWrap(spine::TextureWrap wrap) {
    switch (wrap) {
        default:
        case spine::TextureWrap_ClampToEdge:
            return GL_CLAMP_TO_EDGE;

        case spine::TextureWrap_Repeat:
            return GL_REPEAT;

        case spine::TextureWrap_MirroredRepeat:
            return GL_MIRRORED_REPEAT;
    }
}

void ASpineView::TextureLoader::load(spine::AtlasPage &page, const spine::String &path) {
    auto image = AImage::fromUrl(path.buffer());
    if (!image) {
        throw AException("can't load spine texture atlas: {}"_format(path.buffer()));
    }
    auto texture = new gl::Texture2D;
    texture->tex2D(*image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, parseFilter(page.minFilter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, parseFilter(page.magFilter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, parseWrap(page.uWrap));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, parseWrap(page.vWrap));

    page.width = image->width();
    page.height = image->height();

    page.setRendererObject(texture);
}

void ASpineView::TextureLoader::unload(void *texture) {
    delete static_cast<gl::Texture2D*>(texture);
}

spine::SpineExtension *spine::getDefaultExtension() {
    return new spine::DefaultSpineExtension();
}
