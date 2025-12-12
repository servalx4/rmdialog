#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

class DialogueRenderer {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* bg_texture;
    SDL_Texture* icon_texture;
    TTF_Font* font;
    Mix_Chunk* blip_sound;
    
    struct Args {
        std::string dialogue;
        std::string background = "test.png";
        std::string icon = "testbg.png";
        std::string sound = "text.wav";
        std::string font_path = "font.ttf";
        std::string side = "left";
        int fontsize = 24;
        int width = 800;
        int height = 600;
        double speed = 0.05;
        double delayheavy = 5.0;
        double delaylight = 3.0;
        int iconwidth = 64;
        int iconheight = 64;
        int padding = 20;
        std::string halign = "left";
        std::string valign = "bottom";
    } args;

public:
    DialogueRenderer() : window(nullptr), renderer(nullptr), bg_texture(nullptr), 
                        icon_texture(nullptr), font(nullptr), blip_sound(nullptr) {}
    
    ~DialogueRenderer() {
        cleanup();
    }

    bool parseArgs(int argc, char* argv[]) {
        // Simple command line parsing - for production use, consider a proper library
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            
            if (arg == "--background" && i + 1 < argc) {
                args.background = argv[++i];
            } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " <dialogue> [options]\n\n"
                      << "Required:\n"
                      << "  <dialogue>          The dialogue text to display\n\n"
                      << "Options:\n"
                      << "  --background FILE   Background image (default: test.png)\n"
                      << "  --icon FILE         Character icon (default: testbg.png)\n"
                      << "  --sound FILE        Text blip sound (default: text.wav)\n"
                      << "  --font FILE         Font file (default: font.ttf)\n"
                      << "  --side left|right   Icon side (default: left)\n"
                      << "  --fontsize SIZE     Font size (default: 24)\n"
                      << "  --width WIDTH       Window width (default: 800)\n"
                      << "  --height HEIGHT     Window height (default: 600)\n"
                      << "  --speed SPEED       Text speed (default: 0.05)\n"
                      << "  --delayheavy MULT   Heavy punctuation delay multiplier (default: 5.0)\n"
                      << "  --delaylight MULT   Light punctuation delay multiplier (default: 3.0)\n"
                      << "  --iconsize W H      Icon dimensions (default: 64 64)\n"
                      << "  --padding PIXELS    Padding (default: 20)\n"
                      << "  --halign left|center|right  Horizontal text alignment (default: left)\n"
                      << "  --valign top|middle|bottom  Vertical text alignment (default: bottom)\n"
                      << "  --help, -h          Show this help message\n";
            return false;
            } else if (arg == "--icon" && i + 1 < argc) {
                args.icon = argv[++i];
            } else if (arg == "--sound" && i + 1 < argc) {
                args.sound = argv[++i];
            } else if (arg == "--font" && i + 1 < argc) {
                args.font_path = argv[++i];
            } else if (arg == "--side" && i + 1 < argc) {
                args.side = argv[++i];
            } else if (arg == "--fontsize" && i + 1 < argc) {
                args.fontsize = std::atoi(argv[++i]);
            } else if (arg == "--width" && i + 1 < argc) {
                args.width = std::atoi(argv[++i]);
            } else if (arg == "--height" && i + 1 < argc) {
                args.height = std::atoi(argv[++i]);
            } else if (arg == "--speed" && i + 1 < argc) {
                args.speed = std::atof(argv[++i]);
            } else if (arg == "--delayheavy" && i + 1 < argc) {
                args.delayheavy = std::atof(argv[++i]);
            } else if (arg == "--delaylight" && i + 1 < argc) {
                args.delaylight = std::atof(argv[++i]);
            } else if (arg == "--iconsize" && i + 2 < argc) {
                args.iconwidth = std::atoi(argv[++i]);
                args.iconheight = std::atoi(argv[++i]);
            } else if (arg == "--padding" && i + 1 < argc) {
                args.padding = std::atoi(argv[++i]);
            } else if (arg == "--halign" && i + 1 < argc) {
                args.halign = argv[++i];
            } else if (arg == "--valign" && i + 1 < argc) {
                args.valign = argv[++i];
            } else if (arg.substr(0, 2) != "--") {
                args.dialogue = arg;
            }
        }
        
        if (args.dialogue.empty()) {
            std::cerr << "Error: dialogue text is required\n";
            return false;
        }
        
        return true;
    }

    bool initialize() {
        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
            std::cerr << "SDL init failed: " << SDL_GetError() << std::endl;
            return false;
        }

        // Initialize SDL_image
        if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
            std::cerr << "SDL_image init failed: " << IMG_GetError() << std::endl;
            return false;
        }

        // Initialize SDL_ttf
        if (TTF_Init() == -1) {
            std::cerr << "SDL_ttf init failed: " << TTF_GetError() << std::endl;
            return false;
        }

        // Initialize SDL_mixer
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
            std::cerr << "SDL_mixer init failed: " << Mix_GetError() << std::endl;
            return false;
        }

        // Create window (borderless like pygame.NOFRAME)
        window = SDL_CreateWindow("dialogue", 
                                SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                args.width, args.height, SDL_WINDOW_BORDERLESS);
        if (!window) {
            std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
            return false;
        }

        // Create renderer
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
            std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
            return false;
        }

        return true;
    }

    bool loadResources() {
        // Load background image
        SDL_Surface* bg_surface = IMG_Load(args.background.c_str());
        if (!bg_surface) {
            std::cerr << "Failed to load background: " << IMG_GetError() << std::endl;
            return false;
        }
        bg_texture = SDL_CreateTextureFromSurface(renderer, bg_surface);
        SDL_FreeSurface(bg_surface);

        // Load icon image
        SDL_Surface* icon_surface = IMG_Load(args.icon.c_str());
        if (!icon_surface) {
            std::cerr << "Failed to load icon: " << IMG_GetError() << std::endl;
            return false;
        }
        icon_texture = SDL_CreateTextureFromSurface(renderer, icon_surface);
        SDL_FreeSurface(icon_surface);

        // Load font
        font = TTF_OpenFont(args.font_path.c_str(), args.fontsize);
        if (!font) {
            std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
            return false;
        }

        // Load sound
        blip_sound = Mix_LoadWAV(args.sound.c_str());
        if (!blip_sound) {
            std::cerr << "Failed to load sound: " << Mix_GetError() << std::endl;
            return false;
        }

        return true;
    }

    void drawBackground() {
        SDL_RenderCopy(renderer, bg_texture, nullptr, nullptr);
    }

    void drawIcon() {
        SDL_Rect icon_rect;
        icon_rect.w = args.iconwidth;
        icon_rect.h = args.iconheight;
        
        if (args.side == "left") {
            icon_rect.x = args.padding;
        } else {
            icon_rect.x = args.width - args.iconwidth - args.padding;
        }
        icon_rect.y = args.height - args.iconheight - args.padding;
        
        SDL_RenderCopy(renderer, icon_texture, nullptr, &icon_rect);
    }

    std::vector<std::string> wrapText(const std::string& text) {
        std::vector<std::string> lines;
        std::string current_line = "";
        
        int text_max_width;
        if (args.side == "left") {
            text_max_width = args.width - (args.padding + args.iconwidth + args.padding) - args.padding;
        } else {
            text_max_width = args.width - args.padding - args.iconwidth - 2 * args.padding;
        }
        
        for (char c : text) {
            std::string test_line = current_line + c;
            int w, h;
            TTF_SizeText(font, test_line.c_str(), &w, &h);
            
            if (w > text_max_width && !current_line.empty()) {
                lines.push_back(current_line);
                current_line = std::string(1, c);
            } else {
                current_line = test_line;
            }
        }
        
        if (!current_line.empty()) {
            lines.push_back(current_line);
        }
        
        return lines;
    }

    double getDelay(char c) {
        if (c == '.' || c == '!' || c == '?') {
            return args.speed * args.delayheavy;
        } else if (c == ',' || c == ';' || c == ':') {
            return args.speed * args.delaylight;
        } else {
            return args.speed;
        }
    }

    void renderDialogue() {
        std::vector<std::string> lines = wrapText(args.dialogue);
        
        int line_height = TTF_FontHeight(font) + 5; // 5 is line_spacing
        int total_text_height = lines.size() * line_height - 5;
        
        int y_base;
        if (args.valign == "top") {
            y_base = args.padding;
        } else if (args.valign == "middle") {
            y_base = (args.height - total_text_height) / 2;
        } else { // bottom
            y_base = args.height - total_text_height - args.padding;
        }
        
        int text_x_start;
        if (args.side == "left") {
            text_x_start = args.padding + args.iconwidth + args.padding;
        } else {
            text_x_start = args.padding;
        }
        
        // Draw background and icon once
        drawBackground();
        drawIcon();
        SDL_RenderPresent(renderer);
        
        SDL_Color white = {255, 255, 255, 255};
        
        for (size_t line_index = 0; line_index < lines.size(); line_index++) {
            const std::string& line = lines[line_index];
            
            int line_width, line_height_temp;
            TTF_SizeText(font, line.c_str(), &line_width, &line_height_temp);
            
            int x_base;
            if (args.halign == "left") {
                x_base = text_x_start;
            } else if (args.halign == "center") {
                x_base = (args.width - line_width) / 2;
            } else { // right
                x_base = args.width - line_width - args.padding;
            }
            
            int y_pos = y_base + line_index * line_height;
            std::string rendered_text = "";
            
            for (char c : line) {
                // Handle SDL events
                SDL_Event e;
                while (SDL_PollEvent(&e)) {
                    if (e.type == SDL_QUIT) {
                        return;
                    }
                }
                
                rendered_text += c;
                
                // Clear previous text area and redraw background/icon
                drawBackground();
                drawIcon();
                
                // Render all previous lines completely
                for (size_t prev_line = 0; prev_line < line_index; prev_line++) {
                    SDL_Surface* prev_surface = TTF_RenderText_Solid(font, lines[prev_line].c_str(), white);
                    SDL_Texture* prev_texture = SDL_CreateTextureFromSurface(renderer, prev_surface);
                    
                    int prev_line_width;
                    TTF_SizeText(font, lines[prev_line].c_str(), &prev_line_width, &line_height_temp);
                    
                    int prev_x_base;
                    if (args.halign == "left") {
                        prev_x_base = text_x_start;
                    } else if (args.halign == "center") {
                        prev_x_base = (args.width - prev_line_width) / 2;
                    } else {
                        prev_x_base = args.width - prev_line_width - args.padding;
                    }
                    
                    SDL_Rect prev_rect = {prev_x_base, y_base + (int)prev_line * line_height, prev_surface->w, prev_surface->h};
                    SDL_RenderCopy(renderer, prev_texture, nullptr, &prev_rect);
                    
                    SDL_FreeSurface(prev_surface);
                    SDL_DestroyTexture(prev_texture);
                }
                
                // Render current partial line
                SDL_Surface* text_surface = TTF_RenderText_Solid(font, rendered_text.c_str(), white);
                SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
                
                SDL_Rect text_rect = {x_base, y_pos, text_surface->w, text_surface->h};
                SDL_RenderCopy(renderer, text_texture, nullptr, &text_rect);
                
                SDL_FreeSurface(text_surface);
                SDL_DestroyTexture(text_texture);
                SDL_RenderPresent(renderer);
                
                if (c != ' ') {
                    Mix_PlayChannel(-1, blip_sound, 0);
                }
                
                std::this_thread::sleep_for(std::chrono::duration<double>(getDelay(c)));
            }
        }
    }

    void waitForExit() {
        bool running = true;
        while (running) {
            SDL_Event e;
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) {
                    running = false;
                } else if (e.type == SDL_KEYDOWN) {
                    if (e.key.keysym.sym == SDLK_RETURN) {
                        running = false;
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
        }
    }

    void cleanup() {
        if (blip_sound) Mix_FreeChunk(blip_sound);
        if (font) TTF_CloseFont(font);
        if (icon_texture) SDL_DestroyTexture(icon_texture);
        if (bg_texture) SDL_DestroyTexture(bg_texture);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
    }
};
int main(int argc, char* argv[]) {
    DialogueRenderer renderer;
    
    if (!renderer.parseArgs(argc, argv)) {
        return 1;
    }
    
    if (!renderer.initialize()) {
        return 1;
    }
    
    if (!renderer.loadResources()) {
        return 1;
    }
    
    renderer.renderDialogue();
    renderer.waitForExit();
    
    // Cleanup happens automatically in destructor
    Mix_Quit();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    
    return 0;
}
