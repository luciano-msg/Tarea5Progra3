#include <iostream>
#include <memory>
#include <string>

class Window {
public:
    virtual ~Window() = default;
    virtual void draw() const = 0;
};

class Button {
public:
    virtual ~Button() = default;
    virtual void draw() const = 0;
    virtual void click() const = 0;
};

class Shader {
public:
    virtual ~Shader() = default;
    virtual void compile() const = 0;
};

// ---------- OpenGL ----------
class OpenGLWindow : public Window {
public:
    void draw() const override {
        std::cout << "[OpenGL] glClear + SwapBuffers en contexto GLFW" << std::endl;
    }
};

class OpenGLButton : public Button {
public:
    void draw() const override {
        std::cout << "[OpenGL] Boton dibujado con glDrawArrays(GL_TRIANGLES)" << std::endl;
    }
    void click() const override {
        std::cout << "[OpenGL] Click detectado via callback GLFW" << std::endl;
    }
};

class OpenGLShader : public Shader {
public:
    void compile() const override {
        std::cout << "[OpenGL] Compilando GLSL con glCompileShader + glLinkProgram" << std::endl;
    }
};

// ---------- Vulkan ----------
class VulkanWindow : public Window {
public:
    void draw() const override {
        std::cout << "[Vulkan] vkAcquireNextImageKHR + vkQueuePresentKHR sobre swapchain" << std::endl;
    }
};

class VulkanButton : public Button {
public:
    void draw() const override {
        std::cout << "[Vulkan] Boton grabado en command buffer con vkCmdDraw" << std::endl;
    }
    void click() const override {
        std::cout << "[Vulkan] Click procesado y re-grabado el command buffer" << std::endl;
    }
};

class VulkanShader : public Shader {
public:
    void compile() const override {
        std::cout << "[Vulkan] Cargando SPIR-V con vkCreateShaderModule" << std::endl;
    }
};

// ---------- Fábricas ----------
class RenderFactory {
public:
    virtual ~RenderFactory() = default;
    virtual std::unique_ptr<Window> createWindow() const = 0;
    virtual std::unique_ptr<Button> createButton() const = 0;
    virtual std::unique_ptr<Shader> createShader() const = 0;
};

class OpenGLFactory : public RenderFactory {
public:
    std::unique_ptr<Window> createWindow() const override { return std::make_unique<OpenGLWindow>(); }
    std::unique_ptr<Button> createButton() const override { return std::make_unique<OpenGLButton>(); }
    std::unique_ptr<Shader> createShader() const override { return std::make_unique<OpenGLShader>(); }
};

class VulkanFactory : public RenderFactory {
public:
    std::unique_ptr<Window> createWindow() const override { return std::make_unique<VulkanWindow>(); }
    std::unique_ptr<Button> createButton() const override { return std::make_unique<VulkanButton>(); }
    std::unique_ptr<Shader> createShader() const override { return std::make_unique<VulkanShader>(); }
};

// Cliente: solo conoce RenderFactory
void renderFrame(const RenderFactory& factory) {
    auto window = factory.createWindow();
    auto button = factory.createButton();
    auto shader = factory.createShader();
    shader->compile();
    window->draw();
    button->draw();
    button->click();
}

int main() {
    std::unique_ptr<RenderFactory> factory =
        std::make_unique<OpenGLFactory>();

    auto window = factory->createWindow();
    auto button = factory->createButton();

    window->draw();
    button->draw();

    factory = std::make_unique<VulkanFactory>();

    auto shader = factory->createShader();
    shader->compile();

    std::cout << "\n--- Cliente generico con OpenGL ---" << std::endl;
    renderFrame(OpenGLFactory{});

    std::cout << "\n--- Cliente generico con Vulkan ---" << std::endl;
    renderFrame(VulkanFactory{});
}
