#include <cassert>
#include <iostream>

#include <SFML/Graphics.hpp>

#define FPS_LIMIT 10

struct Triangle {
    size_t vertexIndex[3]{};

    bool drawable = true;

    Triangle(size_t one, size_t two, size_t three) {
        vertexIndex[0] = one;
        vertexIndex[1] = two;
        vertexIndex[2] = three;
    };
};

class Geometry {
    std::vector<sf::Vector2f> mPoints;
    std::vector<Triangle> mTriangles;

    void fillGeometry() {
        mPoints = {sf::Vector2f(361, 179), sf::Vector2f(291, 251), sf::Vector2f(413, 264), sf::Vector2f(240, 158),
                   sf::Vector2f(172, 247), sf::Vector2f(332, 337), sf::Vector2f(483, 353), sf::Vector2f(411, 419),
                   sf::Vector2f(259, 381), sf::Vector2f(507, 160)};

        mTriangles.reserve(11);
        mTriangles.emplace_back(0, 1, 2);
        mTriangles.emplace_back(1, 3, 4);
        mTriangles.emplace_back(0, 1, 3);
        mTriangles.emplace_back(1, 4, 5);
        mTriangles.emplace_back(6, 5, 7);
        mTriangles.emplace_back(2, 1, 5);
        mTriangles.emplace_back(2, 5, 6);
        mTriangles.emplace_back(5, 7, 8);
        mTriangles.emplace_back(9, 2, 0);
        mTriangles.emplace_back(9, 2, 6);
        mTriangles.emplace_back(4, 5, 8);
    }

    void triangulateAll() {
        if(mPoints.size() < 3) {
            return;
        }

        // Reset triangles
        const size_t triCount = mTriangles.size();
        mTriangles.clear();
        mTriangles.reserve(triCount);

        // Build overarching triangle with flag !drawable
        // \todo

        // Insert point by point
        for(auto& pt : mPoints) {
            triangulateOne(pt);
        }
    }

    void triangulateOne(sf::Vector2f) {}

   public:
    Geometry() {
        fillGeometry();
    }

    void insertNewPoint(sf::Vector2f point) {
        mPoints.emplace_back(point.x, point.y);

        // Step of incremental triangulation
        triangulateOne(point);
    }

    const std::vector<sf::Vector2f>& getPoints() const {
        return mPoints;
    }

    const std::vector<Triangle>& getTriangles() const {
        return mTriangles;
    }

    void triangulate() {
        triangulateAll();
    }
};


void handleEvents(sf::RenderWindow& window, Geometry& allGeometry) {
    sf::Event event{};
    sf::Rect<float> triangulateButton(15, 80, 75, 25);
    while(window.pollEvent(event)) {
        switch(event.type) {
        case sf::Event::Closed:
            window.close();
            break;
        case sf::Event::KeyPressed:
            if(event.key.code == sf::Keyboard::Escape) {
                window.close();
            }
            break;
        case sf::Event::MouseButtonPressed: {
            const auto xPos = float(event.mouseButton.x);
            const auto yPos = float(event.mouseButton.y);
            // Triangulate button
            if(triangulateButton.contains(xPos, yPos)) {
                std::cout << "Triangulate!"
                          << "\n";
                allGeometry.triangulate();
            } else if(event.mouseButton.button == sf::Mouse::Right) {
                allGeometry.insertNewPoint(sf::Vector2f(xPos, yPos));
            }
            break;
        }

        default:
            break;
        }
    }
}

void fillUi(std::vector<sf::Text>& uiText, std::vector<sf::RectangleShape>& uiRects, const sf::Font& font) {
    sf::RectangleShape triangulateButton(sf::Vector2f(75, 25));
    triangulateButton.setPosition(15, 80);
    triangulateButton.setFillColor(sf::Color(50, 50, 50));
    triangulateButton.setOutlineColor(sf::Color::White);
    triangulateButton.setOutlineThickness(1);
    uiRects.push_back(triangulateButton);

    sf::Text text;
    text.setFont(font);
    text.setPosition(15, 15);
    text.setCharacterSize(16);
    text.setFillColor(sf::Color::White);
    uiText.push_back(text);

    sf::Text triangulateButtonText;
    triangulateButtonText.setFont(font);
    triangulateButtonText.setPosition(20, 83);
    triangulateButtonText.setCharacterSize(12);
    triangulateButtonText.setString("Triangulate");
    triangulateButtonText.setFillColor(sf::Color::White);
    uiText.push_back(triangulateButtonText);
}

int main() {
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;

    sf::RenderWindow window(sf::VideoMode(800, 600), std::string("AVG project Stepan Hojdar"), sf::Style::Default,
                            settings);
    window.setFramerateLimit(FPS_LIMIT);

    sf::Font font;
    if(!font.loadFromFile("../fonts/OpenSans-Regular.ttf")) {
    }

    sf::CircleShape shape(3.f);
    shape.setFillColor(sf::Color::Green);
    shape.setOrigin(shape.getRadius(), shape.getRadius());

    Geometry allGeometry;

    std::vector<sf::Text> uiText;
    std::vector<sf::RectangleShape> uiRects;
    fillUi(uiText, uiRects, font);

    const auto& points = allGeometry.getPoints();
    const auto& triangles = allGeometry.getTriangles();

    while(window.isOpen()) {
        handleEvents(window, allGeometry);

        window.clear();

        // Draw the points
        for(const auto& pt : allGeometry.getPoints()) {
            shape.setPosition(pt.x, pt.y);
            window.draw(shape);
        }

        // Draw the triangle sides
        for(const Triangle& tri : allGeometry.getTriangles()) {
            if(!tri.drawable) {
                continue;
            }

            sf::Vertex line1[] = {sf::Vertex(points[tri.vertexIndex[0]]), sf::Vertex(points[tri.vertexIndex[1]])};
            sf::Vertex line2[] = {sf::Vertex(points[tri.vertexIndex[1]]), sf::Vertex(points[tri.vertexIndex[2]])};
            sf::Vertex line3[] = {sf::Vertex(points[tri.vertexIndex[2]]), sf::Vertex(points[tri.vertexIndex[0]])};

            window.draw(line1, 2, sf::Lines);
            window.draw(line2, 2, sf::Lines);
            window.draw(line3, 2, sf::Lines);
        }

        // Draw UI
        uiText[0].setString("Triangles: " + std::to_string(triangles.size()) +
                            "\nPoints: " + std::to_string(points.size()));
        for(const auto& rect : uiRects) {
            window.draw(rect);
        }
        for(const auto& text : uiText) {
            window.draw(text);
        }

        window.display();
    }

    return 0;
}
