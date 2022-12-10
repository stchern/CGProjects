#include <render/light.h>
#include <render/raytracer.h>
#include <iostream>


void estimatePi()
{
    const int N = 1000000;
    const double leftBorder = -1.0;
    const double rightBorder = 1.0;
    const double interval = rightBorder - leftBorder;
    int n_internalPoints = 0;
    int n_totalPoints = 0;
    double pi;
    srand((unsigned) time(NULL));

    for (size_t i = 0; i < N; i++) {

            // Randomly generated x and y values
            const double rand_x = leftBorder + double(rand()) / double(RAND_MAX / interval);
            const double rand_y = leftBorder + double(rand()) / double(RAND_MAX / interval);
            // Distance between (x, y) from the origin
            const double origin_dist = rand_x * rand_x + rand_y * rand_y;

            // Checking if (x, y) lies inside the define
            // circle with R=1
            if (origin_dist <= 1)
                n_internalPoints++;

            // Total number of points generated
            n_totalPoints++;

            // estimated pi after this iteration
            pi = double(4 * n_internalPoints) / n_totalPoints;
    }

   std::cout<< "estimated pi: " << pi << std::endl;
   std::cout<< "solid angle of unit circle: " << 4 * pi << std::endl;
}


void estimateSolidAngle()
{
    const int N = 100000;
    const float leftBorder = -1.0;
    const float rightBorder = 1.0;
    const float interval = rightBorder - leftBorder;
    const float r = 1.0;
    int n_internalPoints = 0;
    srand((unsigned) time(NULL));

    for (size_t i = 0; i < N; i++) {

            // Randomly generated x and y values inside hemisphere
            float rand_x = leftBorder + float(rand()) / float(RAND_MAX / interval);
            float rand_y = leftBorder + float(rand()) / float(RAND_MAX / interval);

            // uniformly distributed points on the hemisphere
            const float theta = std::acos(rand_x);
            const float phi = 2.0 * M_PI * rand_y;

            // from spherical coords to Cartesian, (x,y,z) - point on hemisphere
            const float x = r * std::sin(theta) * std::cos(phi);
            const float y = r * std::sin(theta) * std::sin(phi);
            const float z = r * std::cos(theta);

            // Randomly generated x and y values for guide vector from point on hemisphere
            rand_x = leftBorder + float(rand()) / float(RAND_MAX / rand());
            rand_y = leftBorder + float(rand()) / float(RAND_MAX / rand());
            const float rand_z = float(rand()) / float(RAND_MAX / rand());
            Vector3D guideVector{rand_x, rand_y, rand_z};
            guideVector /= guideVector.norm();

            const Vector3D planeNormalVector{0.0, 0.0, 2.0};

            // finding parameter t by formula t = - (A*x + B * y + C * z) / (A * x1 +  B * x2 + C * x3 + D)
            // where (A, B, C) - planeNormalVector, (x, y , z) - point on hemisphere, (x1, x2, x3) - guideVector, D=0

//            Vector3D intersectionPoint = - (planeNormalVector.x * x + planeNormalVector.y * y + planeNormalVector.z * z)
//                    / (planeNormalVector.x * guideVector.x + planeNormalVector.y * guideVector.y + planeNormalVector.z * guideVector.z);

            // because planeNormalVector.x and planeNormalVector.y == 0
            const float t = - (planeNormalVector.z * z) / (planeNormalVector.z * guideVector.z);

            const Vector3D intersectionPoint {x + guideVector.x * t, y + guideVector.y * t, z + guideVector.z * t};
            const double origin_dist = intersectionPoint.x * intersectionPoint.x + intersectionPoint.y * intersectionPoint.y;

            // Checking if (x, y) lies inside the define circle with R=1
            if (origin_dist <= r)
                n_internalPoints++;
    }

   std::cout<< "count of the fraction of the intersection that end up inside circle : " << n_internalPoints << std::endl;
}

std::pair<Color, Point3D> Light::Area::sampleLi(Point3D receiver, Point2D sample) const
{
    const auto [pos, normal] = instance.samplePointAndNormal(sample);
            const Color& Li = instance.material.emittedRadiance;

            Vector3D w = pos - receiver;
            float distance = w.norm();
            if (distance - 0.0f < std::numeric_limits<double>::epsilon())   // to avoid devision by zero
                distance = 1.0f;
            w /= distance;
            const float cosTheta = -dot(w, normal);
            const float totalArea = instance.mesh.getTotalFaceArea();
            const bool isPointTowardsReceiver = (cosTheta > 0)? true : false;
            const float reversePdf = isPointTowardsReceiver * (cosTheta * totalArea) / (distance * distance);
            // TODO: Divide the incident radiance Li by the probability of sampling this direction.
            // The probability needs to be in solid angle from the receivers perspective.
            // See Lighttransport slides 14, 15, (and 45) for converting from area to solid angle.
            // If the sampled triangle does not point towards the receiver, return 0.
            return {Li * reversePdf, pos};
    //    return {Li , pos};
}

Color RayTracer::pathIntegrator(const Scene& scene, const Ray& cameraRay) const
{
    Ray ray{cameraRay};

    ShadingIntersection its {scene, ray};
    if (!its)
        return {};

    auto toWorld = [&its](const auto& v) { return its.shadingFrame.toWorld(v); };
    auto toLocal = [&its](const auto& v) { return its.shadingFrame.toLocal(v); };

    /// the final radiance arriving at the camera
    Color result{0.0f};
    /// attenuation of emitted radiance due to previous intersections (BRDF*cosine/PDF)
    Color throughput{1.0f};

    /// sample area lights or just compute their contribution via brute-force path tracing
    // TODO: enable this and check if your Light::Area::sampleLi function produces the same image
    const bool sampleAreaLights = true;
    bool diffuse = false;

    for (uint32_t depth=1; its && depth<=params.maxDepth; ++depth) {

        /// the material at the current intersection
        const Material& material = scene.getInstances().at(its.instanceIndex).material;
        /// the direction towards the camera
        const Vector3D omegaO = toLocal(-ray.direction);

        // backfaces are black
        if (omegaO.z < 0.0f && !material.isDielectric())
            break;

        // potentially add emitted radiance from an emitter
        // (but only if we did not sample direct light at the previous intersection)
        if ((!sampleAreaLights || !diffuse) && material.isEmitter())
            result += throughput * material.emittedRadiance;

        diffuse = material.isDiffuse() || material.isRoughConductor();

        if (diffuse) {
            // on diffuse surfaces, we can sample direct light
            if (depth < params.maxDepth)
                result += throughput*computeDirectLight(its, omegaO, !sampleAreaLights);

            // TODO: Sample an outgoing ray direction using the Sampler class
            // and update the throughput: Multiply with the BRDF and cosine terms,
            // and divide by the sample's PDF.
            const Vector3D omegaI = Sampler::uniformHemisphere();
            ray = {its.point, toWorld(omegaI)};
            throughput *=
                material.eval(omegaO, omegaI) * omegaI.z * (1.0f / Sampler::uniformHemispherePdf());
        }
        else {
            // on these surfaces, the outgoing ray direction is fixed - we cannot sample it
            const auto [omegaI, fresnel] = specularReflection(material, omegaO);
                    ray = {its.point, toWorld(omegaI)};
            throughput *= fresnel;
        }

        if (throughput.isBlack())
            break;

        its = {scene, ray};
    }

    result.a = 1.0f;

    return result;
}
