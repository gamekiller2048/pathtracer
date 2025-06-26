#include <input/input.hpp>
#include <graphics/window.hpp>
#include <graphics/app.hpp>
#include <graphics/render/opengl/glcontext.hpp>
#include <graphics/render/mesh.hpp>
#include <graphics/geometry/shape2d.hpp>
#include <graphics/camera/firstperson.hpp>
#include <graphics/render/opengl/gltexture2d.hpp>
#include "../../src/graphics/render/opengl/gltexture2dimpl.hpp"
#include <array>
#include <random>

#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_impl_mgl.h>
#include <imgui/imgui_impl_win32.h>

class MyScene : public mgl::Scene
{
public:
    Ref<mgl::Window> window;
    Ref<mgl::RenderContext> context;
    
    Ref<mgl::Mesh> screenQuad;
    Ref<mgl::ShaderProgram> compute;
    Ref<mgl::ShaderProgram> shader;

    Ref<mgl::Texture2D> screenTexture;
    Ref<mgl::CubeMap> cubemap;
    
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
    
        cubemap = context->createCubeMap();

        std::array<std::string, 6> cubeMapTextures = {
            "tests/raytracing/posx.jpg",
            "tests/raytracing/negx.jpg" ,
            "tests/raytracing/negy.jpg" ,
            "tests/raytracing/posy.jpg" ,
            "tests/raytracing/posz.jpg" ,
            "tests/raytracing/negz.jpg"
        };


        screenTexture = context->createTexture2D();
        screenTexture->allocate(window->getWidth(), window->getHeight(), mgl::TextureFormat::RGB, mgl::TextureFormat::RGBA32F);

        shader->bind();
        screenTexture->bind();
        screenTexture->bindUnit();
        screenTexture->uniformSampler(shader, "tex");
        screenTexture->bindImage(0, 0, mgl::Access::READ_WRITE);

        compute->bind();
        cubemap->loadFromFile(cubeMapTextures);
        cubemap->uniformSampler(compute, "u_background");
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

        screenTexture->bindUnit();
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