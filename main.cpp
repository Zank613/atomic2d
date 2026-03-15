#include <array>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdio>

#include "raylib.h"

/**
 * One electron in the Bohr-style visual model.
 */
struct Electron
{
    int shellIndex;
    float orbitRadius;
    float angle;
    float speed;
};

/**
 * One chemical element loaded from elements.txt.
 */
struct Element
{
    int protonCount;
    std::string symbol;
    std::string name;
};

/**
 * Current atom state shown on screen.
 */
struct Atom
{
    int protons;
    int neutrons;
    int electrons;
    std::vector<Electron> electronCloud;
};

/**
 * Simple shell capacities for the visual model.
 */
enum Orbit
{
    K = 2,
    L = 8,
    M = 18,
    N = 32,
    O = 50,
    P = 72,
    Q = 98
};

constexpr int WINDOW_WIDTH = 1600;
constexpr int WINDOW_HEIGHT = 900;

constexpr float PANEL_WIDTH = 300.0f;

constexpr float NUCLEUS_RADIUS = 18.0f;
constexpr float FIRST_ORBIT_RADIUS = 50.0f;
constexpr float ORBIT_SPACING = 28.0f;
constexpr float ELECTRON_RADIUS = 5.0f;

/**
 * Global element lookup by proton count.
 */
static std::unordered_map<int, Element> g_elements;

/**
 * Loads all elements from a text file.
 *
 * Format per line:
 * protonCount|symbol|name
 *
 * Example:
 * 6|C|Carbon
 */
bool LoadElements(const std::string& filePath);

/**
 * Returns the loaded element for a proton count.
 * Returns nullptr if it is not found.
 */
const Element* GetElement(int protonCount);

/**
 * Builds the Wikipedia URL for a proton count.
 * Returns an empty string if the element is unknown.
 */
std::string GetElementWikipediaUrl(int protonCount);

/**
 * Regenerates the Bohr-style electron shells.
 */
void GenerateElectronCloud(Atom& atom);

/**
 * Updates electron angles.
 */
void UpdateElectrons(Atom& atom, float dt);

/**
 * Draws the full atom.
 */
void DrawAtom(const Atom& atom, Vector2 center);

/**
 * Draws the nucleus.
 */
void DrawNucleus(const Atom& atom, Vector2 center);

/**
 * Draws orbit rings and shell labels.
 */
void DrawOrbits(const Atom& atom, Vector2 center);

/**
 * Draws all electrons.
 */
void DrawElectrons(const Atom& atom, Vector2 center);

/**
 * Draws atom information on the left side.
 */
void DrawAtomInfo(const Atom& atom, Vector2 center);

/**
 * Draws the side control panel.
 */
void DrawSidePanel(Atom& atom, bool& electronsChanged, bool& screenshotRequested);

/**
 * Draws a clickable button.
 * Returns true on click.
 */
bool GuiButton(Rectangle rect, const char* text);

/**
 * Draws a simple +/- integer editor.
 * Returns true if the value changed.
 */
bool GuiValueEditor(const char* label, int& value, float x, float y, Color labelColor);

/**
 * Draws a clickable text link.
 * Returns true on click.
 */
bool GuiLink(Rectangle rect, const std::string& text);

int main()
{
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "atomic2d");
    SetTargetFPS(60);
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    if (!LoadElements("elements.txt"))
    {
        TraceLog(LOG_ERROR, "Failed to load elements.txt");
    }

    Atom atom = {};
    atom.protons = 6;
    atom.neutrons = 6;
    atom.electrons = 6;

    GenerateElectronCloud(atom);

    int screenshotCounter = 1;
    float screenshotSavedTimer = 0.0f;
    std::string screenshotSavedText;

    while (!WindowShouldClose())
    {
        const float dt = GetFrameTime();
        bool electronsChanged = false;
        bool screenshotRequested = false;

        UpdateElectrons(atom, dt);

        if (screenshotSavedTimer > 0.0f)
        {
            screenshotSavedTimer -= dt;
            if (screenshotSavedTimer < 0.0f)
            {
                screenshotSavedTimer = 0.0f;
            }
        }

        BeginDrawing();
        {
            ClearBackground(BLACK);

            const float usableWidth = static_cast<float>(GetScreenWidth()) - PANEL_WIDTH;
            Vector2 atomCenter =
            {
                usableWidth * 0.5f,
                static_cast<float>(GetScreenHeight()) * 0.5f
            };

            DrawAtom(atom, atomCenter);
            DrawSidePanel(atom, electronsChanged, screenshotRequested);

            if (screenshotSavedTimer > 0.0f)
            {
                DrawText(
                    screenshotSavedText.c_str(),
                    24,
                    GetScreenHeight() - 40,
                    20,
                    GREEN
                );
            }
        }
        EndDrawing();

        if (electronsChanged)
        {
            GenerateElectronCloud(atom);
        }

        if (screenshotRequested)
        {
            char fileName[64];
            std::snprintf(fileName, sizeof(fileName), "screenshot_%03d.png", screenshotCounter);
            screenshotCounter++;

            TakeScreenshot(fileName);

            screenshotSavedText = std::string("Saved ") + fileName;
            screenshotSavedTimer = 2.0f;
        }
    }

    CloseWindow();
    return 0;
}

bool LoadElements(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        return false;
    }

    g_elements.clear();

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty())
        {
            continue;
        }

        std::stringstream ss(line);
        std::string protonPart;
        std::string symbolPart;
        std::string namePart;

        if (!std::getline(ss, protonPart, '|'))
        {
            continue;
        }

        if (!std::getline(ss, symbolPart, '|'))
        {
            continue;
        }

        if (!std::getline(ss, namePart))
        {
            continue;
        }

        std::stringstream protonStream(protonPart);
        int protonCount = 0;

        if (!(protonStream >> protonCount))
        {
            continue;
        }

        Element element = {};
        element.protonCount = protonCount;
        element.symbol = symbolPart;
        element.name = namePart;

        g_elements[protonCount] = element;
    }

    return true;
}

const Element* GetElement(int protonCount)
{
    const auto it = g_elements.find(protonCount);
    if (it == g_elements.end())
    {
        return nullptr;
    }

    return &it->second;
}

std::string GetElementWikipediaUrl(int protonCount)
{
    const Element* element = GetElement(protonCount);
    if (element == nullptr)
    {
        return "";
    }

    return "https://en.wikipedia.org/wiki/" + element->name;
}

void GenerateElectronCloud(Atom& atom)
{
    atom.electronCloud.clear();

    const std::array<int, 7> shellCapacities = { K, L, M, N, O, P, Q };
    const std::array<float, 7> shellSpeeds = { 1.45f, 1.00f, 0.72f, 0.50f, 0.36f, 0.28f, 0.22f };

    int remainingElectrons = atom.electrons;

    for (int shellIndex = 0; shellIndex < static_cast<int>(shellCapacities.size()); shellIndex++)
    {
        const int capacity = shellCapacities[shellIndex];
        const int electronsInShell = (remainingElectrons < capacity) ? remainingElectrons : capacity;

        if (electronsInShell <= 0)
        {
            break;
        }

        const float orbitRadius = FIRST_ORBIT_RADIUS + ORBIT_SPACING * static_cast<float>(shellIndex);
        const float orbitSpeed = shellSpeeds[shellIndex];

        for (int i = 0; i < electronsInShell; i++)
        {
            Electron electron = {};
            electron.shellIndex = shellIndex;
            electron.orbitRadius = orbitRadius;
            electron.angle = (2.0f * PI / static_cast<float>(electronsInShell)) * static_cast<float>(i);
            electron.speed = orbitSpeed;

            atom.electronCloud.push_back(electron);
        }

        remainingElectrons -= electronsInShell;
    }
}

void UpdateElectrons(Atom& atom, float dt)
{
    for (auto& electron : atom.electronCloud)
    {
        electron.angle += electron.speed * dt;
    }
}

void DrawNucleus(const Atom& atom, Vector2 center)
{
    const int totalNucleons = atom.protons + atom.neutrons;

    if (totalNucleons <= 0)
    {
        DrawCircleV(center, NUCLEUS_RADIUS, DARKGRAY);
        return;
    }

    float spreadRadius = 10.0f + static_cast<float>(totalNucleons) * 0.35f;
    if (spreadRadius > 34.0f)
    {
        spreadRadius = 34.0f;
    }

    int drawnProtons = 0;
    int drawnNeutrons = 0;

    for (int i = 0; i < totalNucleons; i++)
    {
        bool drawProton = false;

        if (drawnProtons < atom.protons && drawnNeutrons < atom.neutrons)
        {
            drawProton = (i % 2 == 0);
        }
        else if (drawnProtons < atom.protons)
        {
            drawProton = true;
        }

        const float angle = (2.0f * PI / static_cast<float>(totalNucleons)) * static_cast<float>(i);
        float localRadius = spreadRadius * 0.45f;

        if (totalNucleons > 1)
        {
            localRadius = spreadRadius * (0.35f + 0.30f * ((i % 3) / 2.0f));
        }

        const float x = center.x + cosf(angle) * localRadius;
        const float y = center.y + sinf(angle) * localRadius;

        if (drawProton)
        {
            DrawCircleV({ x, y }, 7.0f, RED);
            drawnProtons++;
        }
        else
        {
            DrawCircleV({ x, y }, 7.0f, LIGHTGRAY);
            drawnNeutrons++;
        }
    }

    DrawCircleLines(
        static_cast<int>(center.x),
        static_cast<int>(center.y),
        spreadRadius + 8.0f,
        Color { 110, 110, 110, 255 }
    );
}

void DrawOrbits(const Atom& atom, Vector2 center)
{
    std::array<bool, 7> shellUsed = { false, false, false, false, false, false, false };

    for (const auto& electron : atom.electronCloud)
    {
        if (electron.shellIndex >= 0 && electron.shellIndex < static_cast<int>(shellUsed.size()))
        {
            shellUsed[electron.shellIndex] = true;
        }
    }

    for (int shellIndex = 0; shellIndex < static_cast<int>(shellUsed.size()); shellIndex++)
    {
        if (!shellUsed[shellIndex])
        {
            continue;
        }

        const float radius = FIRST_ORBIT_RADIUS + ORBIT_SPACING * static_cast<float>(shellIndex);

        DrawCircleLines(
            static_cast<int>(center.x),
            static_cast<int>(center.y),
            radius,
            Color { 90, 90, 90, 255 }
        );

        const char* shellName = "?";

        switch (shellIndex)
        {
            case 0:
                shellName = "K";
                break;
            case 1:
                shellName = "L";
                break;
            case 2:
                shellName = "M";
                break;
            case 3:
                shellName = "N";
                break;
            case 4:
                shellName = "O";
                break;
            case 5:
                shellName = "P";
                break;
            case 6:
                shellName = "Q";
                break;
            default:
                break;
        }

        DrawText(
            shellName,
            static_cast<int>(center.x + radius + 10.0f),
            static_cast<int>(center.y - 10.0f),
            20,
            GRAY
        );
    }
}

void DrawElectrons(const Atom& atom, Vector2 center)
{
    for (const auto& electron : atom.electronCloud)
    {
        const float x = center.x + cosf(electron.angle) * electron.orbitRadius;
        const float y = center.y + sinf(electron.angle) * electron.orbitRadius;

        DrawCircleV({ x, y }, ELECTRON_RADIUS, BLUE);
    }
}

void DrawAtomInfo(const Atom& atom, Vector2 center)
{
    const int charge = atom.protons - atom.electrons;
    const int massNumber = atom.protons + atom.neutrons;
    const Element* element = GetElement(atom.protons);

    DrawText(TextFormat("Protons: %d", atom.protons), 24, 20, 22, RED);
    DrawText(TextFormat("Neutrons: %d", atom.neutrons), 24, 48, 22, LIGHTGRAY);
    DrawText(TextFormat("Electrons: %d", atom.electrons), 24, 76, 22, BLUE);
    DrawText(TextFormat("Mass Number: %d", massNumber), 24, 116, 22, RAYWHITE);
    DrawText(TextFormat("Charge: %+d", charge), 24, 144, 22, RAYWHITE);

    if (element != nullptr)
    {
        DrawText(
            TextFormat("Element: %s (%s)", element->name.c_str(), element->symbol.c_str()),
            24,
            172,
            22,
            GOLD
        );
    }
    else
    {
        DrawText("Element: Unknown", 24, 172, 22, GOLD);
    }

    DrawCircleLines(
        static_cast<int>(center.x),
        static_cast<int>(center.y),
        NUCLEUS_RADIUS,
        Color { 180, 180, 180, 60 }
    );
}

void DrawAtom(const Atom& atom, Vector2 center)
{
    DrawNucleus(atom, center);
    DrawOrbits(atom, center);
    DrawElectrons(atom, center);
    DrawAtomInfo(atom, center);
}

void DrawSidePanel(Atom& atom, bool& electronsChanged, bool& screenshotRequested)
{
    const float panelX = static_cast<float>(GetScreenWidth()) - PANEL_WIDTH;

    DrawRectangle(
        static_cast<int>(panelX),
        0,
        static_cast<int>(PANEL_WIDTH),
        GetScreenHeight(),
        Color { 20, 20, 28, 255 }
    );

    DrawLine(
        static_cast<int>(panelX),
        0,
        static_cast<int>(panelX),
        GetScreenHeight(),
        Color { 90, 90, 110, 255 }
    );

    DrawText("Atomic Controls", static_cast<int>(panelX + 18.0f), 20, 28, RAYWHITE);

    const float startX = panelX + 20.0f;
    float y = 80.0f;

    GuiValueEditor("Protons", atom.protons, startX, y, RED);

    y += 110.0f;

    GuiValueEditor("Neutrons", atom.neutrons, startX, y, LIGHTGRAY);

    y += 110.0f;

    if (GuiValueEditor("Electrons", atom.electrons, startX, y, BLUE))
    {
        electronsChanged = true;
    }

    y += 120.0f;

    const Rectangle resetButton = { startX, y, 220.0f, 44.0f };
    const Rectangle neutralButton = { startX, y + 58.0f, 220.0f, 44.0f };
    const Rectangle randomButton = { startX, y + 116.0f, 220.0f, 44.0f };
    const Rectangle screenshotButton = { startX, y + 174.0f, 220.0f, 44.0f };

    if (GuiButton(resetButton, "Reset"))
    {
        atom.protons = 6;
        atom.neutrons = 6;
        atom.electrons = 6;
        electronsChanged = true;
    }

    if (GuiButton(neutralButton, "Make Neutral"))
    {
        atom.electrons = atom.protons;
        electronsChanged = true;
    }

    if (GuiButton(randomButton, "Random Atom"))
    {
        atom.protons = GetRandomValue(1, 118);
        atom.neutrons = GetRandomValue(atom.protons, atom.protons + 30);
        atom.electrons = atom.protons;
        electronsChanged = true;
    }

    if (GuiButton(screenshotButton, "Save Screenshot"))
    {
        screenshotRequested = true;
    }

    const int charge = atom.protons - atom.electrons;
    const int massNumber = atom.protons + atom.neutrons;
    const Element* element = GetElement(atom.protons);

    DrawText("Summary", static_cast<int>(startX), static_cast<int>(y + 245.0f), 24, RAYWHITE);

    if (element != nullptr)
    {
        DrawText(
            TextFormat("Element: %s (%s)", element->name.c_str(), element->symbol.c_str()),
            static_cast<int>(startX),
            static_cast<int>(y + 280.0f),
            22,
            GOLD
        );
    }
    else
    {
        DrawText("Element: Unknown", static_cast<int>(startX), static_cast<int>(y + 280.0f), 22, GOLD);
    }

    DrawText(TextFormat("Mass: %d", massNumber), static_cast<int>(startX), static_cast<int>(y + 308.0f), 22, RAYWHITE);
    DrawText(TextFormat("Charge: %+d", charge), static_cast<int>(startX), static_cast<int>(y + 336.0f), 22, RAYWHITE);

    if (element != nullptr)
    {
        const Rectangle linkRect =
        {
            startX,
            y + 377.0f,
            220.0f,
            28.0f
        };

        if (GuiLink(linkRect, "Open Wikipedia"))
        {
            const std::string url = GetElementWikipediaUrl(atom.protons);
            OpenURL(url.c_str());
        }
    }
}

bool GuiButton(Rectangle rect, const char* text)
{
    const Vector2 mouse = GetMousePosition();
    const bool hovered = CheckCollisionPointRec(mouse, rect);
    const bool pressed = hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    Color fillColor = Color { 45, 45, 60, 255 };

    if (hovered)
    {
        fillColor = Color { 75, 75, 95, 255 };
    }

    DrawRectangleRec(rect, fillColor);
    DrawRectangleLinesEx(rect, 2.0f, RAYWHITE);

    const int fontSize = 22;
    const int textWidth = MeasureText(text, fontSize);

    DrawText(
        text,
        static_cast<int>(rect.x + rect.width * 0.5f - textWidth * 0.5f),
        static_cast<int>(rect.y + rect.height * 0.5f - fontSize * 0.5f),
        fontSize,
        RAYWHITE
    );

    return pressed;
}

bool GuiValueEditor(const char* label, int& value, float x, float y, Color labelColor)
{
    bool changed = false;

    DrawText(label, static_cast<int>(x), static_cast<int>(y), 24, labelColor);

    const Rectangle minusButton = { x, y + 34.0f, 42.0f, 42.0f };
    const Rectangle plusButton = { x + 150.0f, y + 34.0f, 42.0f, 42.0f };

    if (GuiButton(minusButton, "-"))
    {
        if (value > 0)
        {
            value--;
            changed = true;
        }
    }

    if (GuiButton(plusButton, "+"))
    {
        value++;
        changed = true;
    }

    DrawText(TextFormat("%d", value), static_cast<int>(x + 88.0f), static_cast<int>(y + 43.0f), 26, RAYWHITE);

    return changed;
}

bool GuiLink(Rectangle rect, const std::string& text)
{
    const Vector2 mouse = GetMousePosition();
    const bool hovered = CheckCollisionPointRec(mouse, rect);
    const bool clicked = hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    const Color textColor = hovered ? SKYBLUE : BLUE;

    DrawText(
        text.c_str(),
        static_cast<int>(rect.x),
        static_cast<int>(rect.y),
        22,
        textColor
    );

    const int textWidth = MeasureText(text.c_str(), 22);

    DrawLine(
        static_cast<int>(rect.x),
        static_cast<int>(rect.y + 24.0f),
        static_cast<int>(rect.x + textWidth),
        static_cast<int>(rect.y + 24.0f),
        textColor
    );

    return clicked;
}