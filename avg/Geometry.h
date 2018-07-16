#pragma once

#include <array>
#include <cassert>
#include <optional>
#include <vector>

#include <SFML/Graphics.hpp>
#include <unordered_map>

using Edge = std::pair<size_t, size_t>;

struct Triangle {
    std::array<size_t, 3> vertexIndex;

    bool drawable = true;

    explicit Triangle(size_t one, size_t two, size_t three) : vertexIndex({one, two, three}), drawable(true){};
};

struct pairhash {
   public:
    template <typename T, typename U>
    std::size_t operator()(const std::pair<T, U>& x) const {
        return std::hash<T>()(x.first) ^ std::hash<U>()(x.second);
    }
};

class Geometry {
    std::vector<sf::Vector2f> mPoints;
    std::vector<Triangle> mTriangles;

    // Method taken from https://github.com/Bl4ckb0ne/delaunay-triangulation
    static bool circumCircleContains(const sf::Vector2f& v, const sf::Vector2f& p1, const sf::Vector2f& p2,
                                     const sf::Vector2f& p3) {
        const float ab = p1.x * p1.x + p1.y * p1.y;
        const float cd = p2.x * p2.x + p2.y * p2.y;
        const float ef = p3.x * p3.x + p3.y * p3.y;

        const float circum_x = (ab * (p3.y - p2.y) + cd * (p1.y - p3.y) + ef * (p2.y - p1.y)) /
                               (p1.x * (p3.y - p2.y) + p2.x * (p1.y - p3.y) + p3.x * (p2.y - p1.y));
        const float circum_y = (ab * (p3.x - p2.x) + cd * (p1.x - p3.x) + ef * (p2.x - p1.x)) /
                               (p1.y * (p3.x - p2.x) + p2.y * (p1.x - p3.x) + p3.y * (p2.x - p1.x));

        const auto distance = [](const sf::Vector2f p, const sf::Vector2f v) -> float {
            float dx = p.x - v.x;
            float dy = p.y - v.y;
            return dx * dx + dy * dy;
        };

        const sf::Vector2f circum(0.5f * (circum_x), 0.5f * (circum_y));
        const float circum_radius = distance(p1, circum);
        const float dist = distance(v, circum);
        return dist <= circum_radius;
    }

    void fillGeometry() {
        // Pre-fill the overlapping triangle
        mPoints = {};
        mPoints.emplace_back(sf::Vector2f(400, -1000));
        mPoints.emplace_back(sf::Vector2f(-400, 700));
        mPoints.emplace_back(sf::Vector2f(1200, 700));
        const size_t bigTop = mPoints.size() - 3;
        const size_t bigLeft = mPoints.size() - 2;
        const size_t bigRight = mPoints.size() - 1;
        assert(mPoints[bigTop].x == 400 && mPoints[bigTop].y == -1000);
        assert(mPoints[bigLeft].x == -400 && mPoints[bigLeft].y == 700);
        assert(mPoints[bigRight].x == 1200 && mPoints[bigLeft].y == 700);
        mTriangles.emplace_back(bigTop, bigLeft, bigRight);
        mTriangles[0].drawable = false;
    }

    struct EdgeSearchResult {
        size_t triangleIndex = std::numeric_limits<size_t>::max();
        size_t vertexIndex1 = std::numeric_limits<size_t>::max();
        size_t vertexIndex2 = std::numeric_limits<size_t>::max();

        EdgeSearchResult() = default;
        EdgeSearchResult(size_t a, size_t b, size_t c) : triangleIndex(a), vertexIndex1(b), vertexIndex2(c){};
    };

    std::vector<EdgeSearchResult> findTriangleWithEdge(const Edge& edge) const {
        const size_t vert1 = edge.first;
        const size_t vert2 = edge.second;

        std::vector<EdgeSearchResult> resultList;

        for(size_t i = 0; i < mTriangles.size(); ++i) {
            const Triangle& tri = mTriangles[i];
            bool found1 = false;
            bool found2 = false;

            if(tri.vertexIndex[0] == vert1 || tri.vertexIndex[1] == vert1 || tri.vertexIndex[2] == vert1) {
                found1 = true;
            }
            if(tri.vertexIndex[0] == vert2 || tri.vertexIndex[1] == vert2 || tri.vertexIndex[2] == vert2) {
                found2 = true;
            }
            if(found1 && found2) {
                EdgeSearchResult result;
                result.triangleIndex = i;
                if(tri.vertexIndex[0] == vert1) {
                    result.vertexIndex1 = 0;
                } else if(tri.vertexIndex[1] == vert1) {
                    result.vertexIndex1 = 1;
                } else if(tri.vertexIndex[2] == vert1) {
                    result.vertexIndex1 = 2;
                } else {
                    assert(false);
                }

                if(tri.vertexIndex[0] == vert2) {
                    result.vertexIndex2 = 0;
                } else if(tri.vertexIndex[1] == vert2) {
                    result.vertexIndex2 = 1;
                } else if(tri.vertexIndex[2] == vert2) {
                    result.vertexIndex2 = 2;
                } else {
                    assert(false);
                }
                resultList.push_back(result);
            }
        }
        assert(resultList.size() <= 2 && !resultList.empty());
        return resultList;
    }

    std::vector<Edge> flip(size_t triangleIndex, const size_t vert1, const size_t vert2) {
        const Triangle& triangle = mTriangles[triangleIndex];
        const size_t vertexIndex1 = triangle.vertexIndex[vert1];
        const size_t vertexIndex2 = triangle.vertexIndex[vert2];
        const size_t aIndex = triangle.vertexIndex[3 - vert1 - vert2];  // 0 1 2, I want the third, sum = 3

        // Find second triangle with triangle[vert1] and triangle[vert2]
        std::vector<EdgeSearchResult> incidents = findTriangleWithEdge(Edge(vertexIndex1, vertexIndex2));

        // If there is only one, no flipping required
        if(incidents.size() == 1) {
            assert(incidents[0].triangleIndex == triangleIndex);
            return std::vector<Edge>();
        }

        // Find which of the 2 found is new
        size_t whichTriIsNew = std::numeric_limits<size_t>::max();
        if(incidents[0].triangleIndex == triangleIndex) {
            whichTriIsNew = 1;
        } else if(incidents[1].triangleIndex == triangleIndex) {
            whichTriIsNew = 0;
        } else {
            assert(false);
        }
        assert(whichTriIsNew != std::numeric_limits<size_t>::max());

        const size_t neighbourIndex = incidents[whichTriIsNew].triangleIndex;  // index into mTriangles
        assert(neighbourIndex < mTriangles.size());
        const Triangle& neighbour = mTriangles[neighbourIndex];

        // Check condition
        const size_t dIndex =
            neighbour.vertexIndex[3 - incidents[whichTriIsNew].vertexIndex1 - incidents[whichTriIsNew].vertexIndex2];
        assert(dIndex < mPoints.size());
        const bool flipEdge = circumCircleContains(mPoints[dIndex], mPoints[triangle.vertexIndex[0]],
                                                   mPoints[triangle.vertexIndex[1]], mPoints[triangle.vertexIndex[2]]);

        // If false, return
        if(!flipEdge) {
            return std::vector<Edge>();
        }
        // If true
        else {
            // Flip
            if(triangleIndex > neighbourIndex) {
                mTriangles.erase(mTriangles.begin() + triangleIndex);
                mTriangles.erase(mTriangles.begin() + neighbourIndex);
            } else {
                mTriangles.erase(mTriangles.begin() + neighbourIndex);
                mTriangles.erase(mTriangles.begin() + triangleIndex);
            }

            mTriangles.emplace_back(aIndex, vertexIndex1, dIndex);
            mTriangles.emplace_back(aIndex, dIndex, vertexIndex2);

            // And test the neighbouring four! triangles too
            std::vector<Edge> returnVal;
            returnVal.emplace_back(aIndex, vertexIndex1);
            returnVal.emplace_back(vertexIndex1, dIndex);
            returnVal.emplace_back(dIndex, vertexIndex2);
            returnVal.emplace_back(vertexIndex2, aIndex);
            return returnVal;
        }
    }

    static float dot(const sf::Vector2f left, const sf::Vector2f right) {
        return left.x * right.x + left.y * right.y;
    }

    // Compute barycentric coordinates of 'point' in 'triangle' and check if it is inside. Return coordinates if inside,
    // empty if outside. Code based on the following link. Edited to not only compute but also check the viability.
    // https://gamedev.stackexchange.com/questions/23743/whats-the-most-efficient-way-to-find-barycentric-coordinates,
    std::optional<std::array<float, 3>> checkBarycentricCoordinates(const Triangle& triangle, const sf::Vector2f& point,
                                                                    const float tolerance = 0.0000001f) const {
        const sf::Vector2f& a = mPoints[triangle.vertexIndex[0]];
        const sf::Vector2f& b = mPoints[triangle.vertexIndex[1]];
        const sf::Vector2f& c = mPoints[triangle.vertexIndex[2]];

        const sf::Vector2f v0 = b - a;
        const sf::Vector2f v1 = c - a;
        const sf::Vector2f v2 = point - a;

        const float d00 = dot(v0, v0);
        const float d01 = dot(v0, v1);
        const float d11 = dot(v1, v1);
        const float d20 = dot(v2, v0);
        const float d21 = dot(v2, v1);
        const float denominator = d00 * d11 - d01 * d01;

        if(denominator == 0) {
            return {};
        }

        const float denominatorFraction = 1.f / denominator;
        const float v = (d11 * d20 - d01 * d21) * denominatorFraction;
        const float w = (d00 * d21 - d01 * d20) * denominatorFraction;

        // The point is inside the triangle IFF u + v + w == 1 && 0 <= u,v,w <= 1
        if(std::min(v, w) >= -tolerance && std::max(v, w) <= 1.f + tolerance && v + w <= 1.f + tolerance) {
            return std::array<float, 3>({1.0f - v - w, v, w});
        } else {
            return {};
        }
    }

    // Returns the index of the triangle 'point' lies in
    std::optional<size_t> findTriangleWithPoint(const sf::Vector2f point) const {
        for(size_t i = 0; i < mTriangles.size(); ++i) {
            const auto& triangle = mTriangles[i];
            const auto coordsMaybe = checkBarycentricCoordinates(triangle, point);
            if(coordsMaybe.has_value()) {
                return i;
            }
        }
        return {};
    }

    std::optional<std::array<Edge, 3>> insertPointIntoTriangle(const sf::Vector2f point) {
        if(mPoints.size() < 3) {
            return {};
        } else if(mPoints.size() == 3) {
            mTriangles.emplace_back(0, 1, 2);
            return {};
        }
        const auto indexMaybe = findTriangleWithPoint(point);
        size_t insideTriangleInd = 0;
        if(!indexMaybe.has_value()) {
            return {};
        } else {
            insideTriangleInd = indexMaybe.value();
        }
        const size_t priorTriSize = mTriangles.size();
        assert(insideTriangleInd < mTriangles.size());

        const std::array<size_t, 3> vertexIndices = mTriangles[insideTriangleInd].vertexIndex;
        mTriangles.erase(mTriangles.begin() + insideTriangleInd);
        assert(mPoints.back().x == point.x);
        assert(mPoints.back().y == point.y);

        const size_t pointIndex = mPoints.size() - 1;
        mTriangles.emplace_back(vertexIndices[0], vertexIndices[1], pointIndex);
        mTriangles.emplace_back(pointIndex, vertexIndices[1], vertexIndices[2]);
        mTriangles.emplace_back(vertexIndices[0], pointIndex, vertexIndices[2]);

        assert(priorTriSize + 2 == mTriangles.size());
        return std::array<Edge, 3>({Edge({vertexIndices[0], vertexIndices[1]}),
                                    Edge({vertexIndices[1], vertexIndices[2]}),
                                    Edge({vertexIndices[0], vertexIndices[2]})});
    }

    void queueFlip(const std::array<Edge, 3>& edgesToCheck) {
        std::vector<Edge> queue;
        std::copy(edgesToCheck.begin(), edgesToCheck.end(), std::back_inserter(queue));
        assert(queue.size() == 3);

        while(!queue.empty()) {
            // check one edge
            const Edge current = queue.back();
            const EdgeSearchResult result = findTriangleWithEdge(current)[0];
            std::vector<Edge> newEdgesToFlip = flip(result.triangleIndex, result.vertexIndex1, result.vertexIndex2);

            // remove from list
            queue.pop_back();

            // add new flipped
            for(Edge& e : newEdgesToFlip) {
                queue.push_back(e);
            }
        }
    }

    void insertOnePoint(const sf::Vector2f point) {
        mPoints.emplace_back(point.x, point.y);

        // Step of incremental triangulation
        const auto newTrianglesMaybe = insertPointIntoTriangle(point);
        if(!newTrianglesMaybe.has_value()) {
            return;
        }
        const std::array<Edge, 3> newTriangles = newTrianglesMaybe.value();
        queueFlip(newTriangles);
    }

   public:
    Geometry() {
        fillGeometry();
    }

    void insertPoint(const sf::Vector2f point) {
        insertOnePoint(point);
    }

    const std::vector<sf::Vector2f>& getPoints() const {
        return mPoints;
    }

    std::vector<Triangle>& getTriangles() {
        return mTriangles;
    }

    void triangulate() {
        if(mPoints.size() < 3) {
            return;
        }

        // Reset triangles
        mTriangles.clear();

        // Move points
        auto oldPoints = std::move(mPoints);
        mPoints.clear();
        mPoints.reserve(oldPoints.size());

        // Insert point by point
        for(auto& pt : oldPoints) {
            insertOnePoint(pt);
        }
    }
        
    void reset()
    {
        mPoints.clear();
        mTriangles.clear();
        fillGeometry();
    }
};
