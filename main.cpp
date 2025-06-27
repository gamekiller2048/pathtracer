#include <input/input.hpp>
#include <graphics/window.hpp>
#include <graphics/app.hpp>
#include <graphics/render/opengl/glcontext.hpp>
#include <graphics/render/mesh.hpp>
#include <graphics/geometry/shape2d.hpp>
#include <graphics/camera/firstperson.hpp>
#include <graphics/render/framebuffer.hpp>
#include <array>
#include <random>

class MyScene : public mgl::Scene
{
public:
    Ref<mgl::Window> window;
    Ref<mgl::RenderContext> context;
    
    Ref<mgl::Mesh> screenQuad;
    Ref<mgl::ShaderProgram> compute;
    Ref<mgl::ShaderProgram> shader;
    Ref<mgl::ShaderProgram> bloomShader;

    Ref<mgl::Texture2D> screenTexture;
    Ref<mgl::Texture2D> brightnessScreenTexture;
    Ref<mgl::CubeMap> cubemap;

    Ref<mgl::FrameBuffer> bloomFramebuffer;

    Ref<mgl::Texture2D> pingBloomTexture;
    Ref<mgl::Texture2D> pongBloomTexture;
    
    mgl::FirstPersonCamera camera;

    uint frameCount = 0;

    std::random_device randDevice;
    std::mt19937 rng;
    std::uniform_real_distribution<float> randDist;

    mml::mat4 prevCameraView;

    MyScene(const Ref<mgl::Window>& window) :
        window(window),
        rng(randDevice()),
        randDist(0.0f, 1.0f)
    {
        context = window->getContext();
        context->setDepthTest(true);


        screenQuad = context->createMesh();
        screenQuad->setGeometry(mgl::gl::Shape2D().SquareUV());

        shader = context->createShaderProgram();
        shader->attachGLSLShadersFromFile("tests/raytracing/shader.vert", "tests/raytracing/shader.frag");

        compute = context->createShaderProgram();
        compute->attachGLSLShadersFromFile("", "", "", "tests/raytracing/shader.comp");

        bloomShader = context->createShaderProgram();
        bloomShader->attachGLSLShadersFromFile("tests/raytracing/shader.vert", "tests/raytracing/bloom.frag");
    

        cubemap = context->createCubeMap();
        std::array<std::string, 6> cubeMapTextures = {
            "tests/raytracing/posx.jpg",
            "tests/raytracing/negx.jpg" ,
            "tests/raytracing/negy.jpg" ,
            "tests/raytracing/posy.jpg" ,
            "tests/raytracing/posz.jpg" ,
            "tests/raytracing/negz.jpg"
        };
        cubemap->loadFromFile(cubeMapTextures);

        // create screen and brightness textures
        screenTexture = context->createTexture2D();
        screenTexture->allocate(window->getWidth(), window->getHeight(), mgl::TextureFormat::RGB, mgl::TextureFormat::RGBA32F);

        brightnessScreenTexture = context->createTexture2D();
        brightnessScreenTexture->allocate(window->getWidth(), window->getHeight(), mgl::TextureFormat::RGB, mgl::TextureFormat::RGBA32F);

        // create bloom framebuffer + textures
        bloomFramebuffer = context->createFrameBuffer();

        pingBloomTexture = context->createTexture2D();
        pingBloomTexture->allocate(window->getWidth(), window->getHeight(), mgl::TextureFormat::RGB, mgl::TextureFormat::RGBA32F);

        pongBloomTexture = context->createTexture2D();
        pongBloomTexture->allocate(window->getWidth(), window->getHeight(), mgl::TextureFormat::RGB, mgl::TextureFormat::RGBA32F);

        // bind texture units to shader
        shader->bind();
        screenTexture->uniformSampler(shader, "u_tex");
        pongBloomTexture->uniformSampler(shader, "u_bloom");

        // bind image unit and background to compute
        compute->bind();
        cubemap->uniformSampler(compute, "u_background");

        screenTexture->bindImage(0, 0, mgl::Access::READ_WRITE);
        brightnessScreenTexture->bindImage(1, 0, mgl::Access::READ_WRITE);
    }

    void update()
    {
        context->clearColor();
        context->clearDepth();
        context->viewport(0, 0, window->getWidth(), window->getHeight());
        
        camera.perspective(45.0f, 0.1f, 1000.0f, window->getSize());
        camera.BasicFirstPersonView(window);

        if(camera.view != prevCameraView)
            frameCount = 0;

        prevCameraView = camera.view;

        compute->bind();
        compute->uniform("u_resolution", (mml::vec2)window->getSize());
        compute->uniform("u_cameraPos", camera.pos);
        compute->uniform("u_cameraProj", camera.projection);
        compute->uniform("u_cameraView", camera.view);
        compute->uniform("u_seed", (float)randDist(rng));

        frameCount++;
        compute->uniform("u_frameCount", (int)frameCount);
        compute->compute(mml::uvec3(window->getWidth() / 16, window->getWidth() / 16, 1));
        compute->computeBarrier(mgl::MemoryBarrier::ALL);

        // copy brightness texture to ping
        bloomFramebuffer->bind();
        bloomShader->bind();
        brightnessScreenTexture->uniformSampler(bloomShader, "u_tex");
        pingBloomTexture->bindUnit();
        bloomFramebuffer->addRenderTarget(pingBloomTexture, mgl::FrameBufferAttachment{mgl::FrameBufferAttachmentType::COLOR, 0}, 0);
        screenQuad->draw(bloomShader);

        // ping-pong framebuffer pass for bloom
        uint amount = 9;
        for(uint i = 0; i < amount; i++)
        {
            bloomShader->bind();
            bloomShader->uniform("u_horizontal", i % 2);

            if(i % 2 == 0) {
                pongBloomTexture->bindUnit();
                bloomFramebuffer->addRenderTarget(pongBloomTexture, mgl::FrameBufferAttachment{mgl::FrameBufferAttachmentType::COLOR, 0}, 0);
                pingBloomTexture->uniformSampler(bloomShader, "u_tex");
            }
            else {
                pingBloomTexture->bindUnit();
                bloomFramebuffer->addRenderTarget(pingBloomTexture, mgl::FrameBufferAttachment{mgl::FrameBufferAttachmentType::COLOR, 0}, 0);
                pongBloomTexture->uniformSampler(bloomShader, "u_tex");
            }

            screenQuad->draw(bloomShader);
        }

        bloomFramebuffer->unbind();
        
        shader->bind();
        screenQuad->draw(shader);

    }
};

int main()
{
    Ref<mgl::App> app = CreateRef<mgl::App>(mgl::RenderApi::OPENGL);
    Ref<mgl::Window> window = CreateRef<mgl::Window>("hello world", 1200, 700);
    
    Ref<mgl::GLContext> context = CreateRef<mgl::GLContext>();
    context->create(window, 3, 3);
    window->setContext(context);

    Ref<mgl::Scene> scene = CreateRef<MyScene>(window);
    window->addScene("main", scene);
    window->setScene("main");

    app->addWindow(window);
    app->run();
    return 0;
}