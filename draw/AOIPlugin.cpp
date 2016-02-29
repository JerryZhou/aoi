
#include <sstream>
#include "draw.h"
#include "aoi.h"

#ifdef WIN32
// Windows defines these as macros :(
#undef min
#undef max
#endif

#ifndef NO_LQ_BIN_STATS
#include <iomanip> // for setprecision
#include <limits> // for numeric_limits::max()
#endif // NO_LQ_BIN_STATS


namespace {
    static iid aoiid = 0;
    static imap* aoimap = NULL;
    
    // Include names declared in the OpenSteer namespace into the
    // namespaces to search to find names.
    using namespace Draw;
    
    typedef enum UnitBehavior {
        UnitBehavior_Searching = 1,
    }UnitBehavior;
    
    
    class AOIUnit : public Draw::SimpleVehicle {
        
    public:
        
        iunit* aoiunit;
        isearchresult *searchresult;
        Color color;
        float searchradis;
        int behavior;
        
        AOIUnit(){
            aoiunit = NULL;
            searchresult = NULL;
            behavior = 0;
            searchradis = 0;
            
            color = gGreen;
            reset ();
            makeunit();
        }
        virtual ~AOIUnit() {
            this->aoiunit->userdata.up1 = NULL;
            imapremoveunit(aoimap, this->aoiunit);
            ifreeunit(aoiunit);
            isearchresultfree(searchresult);
        }
        // searching
        void makeSearching() {
            if (searchresult != NULL) {
                return;
            }
            color = Color(1.0 * rand() / INT_MAX, 1.0 * rand() / INT_MAX, 1.0 * rand() / INT_MAX);
            searchradis = 10;
            behavior |= UnitBehavior_Searching;
            if (searchresult == NULL) {
                this->searchresult = isearchresultmake();
            }
        }
        // draw this boid into the scene
        void draw (void)
        {
            drawBasic3dSphericalVehicle (*this, color);
            // draw3dCircle(this->radius(), this->position() + Vec3(0, 1, 0), this->up(), this->color, 50);
            // drawTrail ();
            
            // draw the search circle
            if (behavior & UnitBehavior_Searching) {
                draw3dCircle(searchradis, this->position() + Vec3(0, 1, 0), this->up(), gRed, 50);
            }
            
            int id = (int)this->aoiunit->id;
            std::ostringstream message;
            message.precision(3);
            message << "id:(" << id <<")" << std::ends;// <<" Code:"<<this->aoiunit->code.code;
            //message << " pos: " << "(" << this->position().x <<"," <<this->position().z <<")" <<  std::ends;
 
            draw2dTextAt3dLocation(message, this->position(), gYellow, drawGetWindowWidth(), drawGetWindowHeight());
            
            // we got search result
            if (this->searchresult && ireflistlen(this->searchresult->units)) {
                irefjoint *joint = ireflistfirst(this->searchresult->units);
                while (joint) {
                    iunit *unit = icast(iunit, joint->value);
                    AOIUnit *searchunit = (AOIUnit*)unit->userdata.up1;
                    if (searchunit != this && searchunit) {
                        draw3dCircle(searchunit->radius(),
                                     searchunit->position() + Vec3(0, 1, 0),
                                     searchunit->up(), gOrange, 10);
                    }
                    joint = joint->next;
                }
            }
        }
        
        // per frame simulation update
        void update (const float currentTime, const float elapsedTime)
        {
            //OPENSTEER_UNUSED_PARAMETER(currentTime);
            
            // steer to flock and avoid obstacles if any
            applySteeringForce (steerToFlock (), elapsedTime);
            
            // wrap around to contrain boid within the spherical boundary
            sphericalWrapAround ();
            
            // notify proximity database that our position has changed
            // proximityToken->updateForNewPosition (position());
            
            // update the aoi unit
            this->updateAOIUnit();
            
            // search it
            if (this->behavior & UnitBehavior_Searching) {
                imapsearchfromunit(aoimap, this->aoiunit, this->searchresult, this->searchradis);
            }
        }
        
        // reset state
        void reset (void)
        {
            // reset the vehicle
            SimpleVehicle::reset ();
            
            // steering force is clipped to this magnitude
            setMaxForce (27);
            
            // velocity is clipped to this magnitude
            setMaxSpeed (9);
            
            // initial slow speed
            setSpeed (maxSpeed() * 0.3f);
            
            // randomize initial orientation
            regenerateOrthonormalBasisUF (Draw::randomVectorOnUnitRadiusXZDisk ().normalize());
            
            // randomize initial position
            setPosition (Draw::randomVectorOnUnitRadiusXZDisk () * 20 + Vec3(0, 1.0, 0));
            
            setRadius(2.0f);
        }
        
        void makeunit() {
            // add unit if need
            if (this->aoiunit == NULL) {
                // make unit
                this->aoiunit = imakeunit(aoiid++, this->position().x, this->position().z);
                // save this in userdata: up1
                this->aoiunit->userdata.up1 = this;
                // add unit to map
                imapaddunit(aoimap, this->aoiunit);
                
                // search it
                if (this->aoiunit->id % 10 == 0) {
                    this->makeSearching();
                }
            }
        }
        
        void updateAOIUnit() {
            // new pos
            this->aoiunit->pos.x = this->position().x;
            this->aoiunit->pos.y = this->position().z;
            // update the pos in map
            imapupdateunit(aoimap, this->aoiunit);
        }
        
        // basic flocking
        Vec3 steerToFlock (void)
        {
            // avoid obstacles if needed
            // XXX this should probably be moved elsewhere
            const Vec3 avoidance = steerToAvoidObstacles (1.0f, obstacles);
            if (avoidance != Vec3::zero) return avoidance;
            
            const float separationRadius =  5.0f;
            const float separationAngle  = -0.707f;
            const float separationWeight =  12.0f;
            
            const float alignmentRadius = 7.5f;
            const float alignmentAngle  = 0.7f;
            const float alignmentWeight = 8.0f;
            
            const float cohesionRadius = 9.0f;
            const float cohesionAngle  = -0.15f;
            const float cohesionWeight = 8.0f;
            
            const float maxRadius = maxXXX (separationRadius,
                                            maxXXX (alignmentRadius,
                                                    cohesionRadius));
            
            // find all flockmates within maxRadius using proximity database
            neighbors.clear();
            //proximityToken->findNeighbors (position(), maxRadius, neighbors);
            
#ifndef NO_LQ_BIN_STATS
            // maintain stats on max/min/ave neighbors per boids
            size_t count = neighbors.size();
            if (maxNeighbors < count) maxNeighbors = count;
            if (minNeighbors > count) minNeighbors = count;
            totalNeighbors += count;
#endif // NO_LQ_BIN_STATS
            
            // determine each of the three component behaviors of flocking
            const Vec3 separation = steerForSeparation (separationRadius,
                                                        separationAngle,
                                                        neighbors);
            const Vec3 alignment  = steerForAlignment  (alignmentRadius,
                                                        alignmentAngle,
                                                        neighbors);
            const Vec3 cohesion   = steerForCohesion   (cohesionRadius,
                                                        cohesionAngle,
                                                        neighbors);
            
            // apply weights to components (save in variables for annotation)
            const Vec3 separationW = separation * separationWeight;
            const Vec3 alignmentW = alignment * alignmentWeight;
            const Vec3 cohesionW = cohesion * cohesionWeight;
            
            // annotation
            // const float s = 0.1;
            // annotationLine (position, position + (separationW * s), gRed);
            // annotationLine (position, position + (alignmentW  * s), gOrange);
            // annotationLine (position, position + (cohesionW   * s), gYellow);
            
            return separationW + alignmentW + cohesionW;
        }
        
        
        // constrain this boid to stay within sphereical boundary.
        void sphericalWrapAround (void)
        {
            // when outside the sphere
            if (position().length() > worldRadius)
            {
                // wrap around (teleport)
                Vec3 newpos = position().sphericalWrapAround(Vec3::zero, worldRadius);
                newpos.setYtoZero();
                newpos.y = 1.0;
                setPosition (newpos);
                
                if (this == App::selectedVehicle)
                {
                    App::position3dCamera
                    (*App::selectedVehicle);
                    App::camera.doNotSmoothNextMove ();
                }
            }
        }
        
        
        // ---------------------------------------------- xxxcwr111704_terrain_following
        // control orientation for this boid
        void regenerateLocalSpace (const Vec3& newVelocity,
                                   const float elapsedTime)
        {
            // 3d flight with banking
            regenerateLocalSpaceForBanking (newVelocity, elapsedTime);
            
            // // follow terrain surface
            // regenerateLocalSpaceForTerrainFollowing (newVelocity, elapsedTime);
        }
        
        
        // XXX experiment:
        // XXX   herd with terrain following
        // XXX   special case terrain: a sphere at the origin, radius 40
        void regenerateLocalSpaceForTerrainFollowing  (const Vec3& newVelocity,
                                                       const float /* elapsedTime */)
        {
            
            // XXX this is special case code, these should be derived from arguments //
            const Vec3 surfaceNormal = position().normalize();                       //
            const Vec3 surfacePoint = surfaceNormal * 40.0f;                         //
            // XXX this is special case code, these should be derived from arguments //
            
            const Vec3 newUp = surfaceNormal;
            const Vec3 newPos = surfacePoint;
            const Vec3 newVel = newVelocity.perpendicularComponent(newUp);
            const float newSpeed = newVel.length();
            const Vec3 newFor = newVel / newSpeed;
            
            setSpeed (newSpeed);
            setPosition (newPos);
            setUp (newUp);
            setForward (newFor);
            setUnitSideFromForwardAndUp ();
        }
        
        // group of all obstacles to be avoided by each Boid
        static ObstacleGroup obstacles;
        
        // a pointer to this boid's interface object for the proximity database
        // ProximityToken* proximityToken;
        
        // allocate one and share amoung instances just to save memory usage
        // (change to per-instance allocation to be more MP-safe)
        static AVGroup neighbors;
        
        static float worldRadius;
        
        // xxx perhaps this should be a call to a general purpose annotation for
        // xxx "local xxx axis aligned box in XZ plane" -- same code in in
        // xxx CaptureTheFlag.cpp
        void annotateAvoidObstacle (const float minDistanceToCollision)
        {
            const Vec3 boxSide = side() * radius();
            const Vec3 boxFront = forward() * minDistanceToCollision;
            const Vec3 FR = position() + boxFront - boxSide;
            const Vec3 FL = position() + boxFront + boxSide;
            const Vec3 BR = position()            - boxSide;
            const Vec3 BL = position()            + boxSide;
            const Color white (1,1,1);
            annotationLine (FR, FL, white);
            annotationLine (FL, BL, white);
            annotationLine (BL, BR, white);
            annotationLine (BR, FR, white);
        }
        
#ifndef NO_LQ_BIN_STATS
        static size_t minNeighbors, maxNeighbors, totalNeighbors;
#endif // NO_LQ_BIN_STATS
    };
    
    AVGroup AOIUnit::neighbors;
    float AOIUnit::worldRadius = 80.0f;
    ObstacleGroup AOIUnit::obstacles;
#ifndef NO_LQ_BIN_STATS
    size_t AOIUnit::minNeighbors, AOIUnit::maxNeighbors, AOIUnit::totalNeighbors;
#endif // NO_LQ_BIN_STATS
    
    // ----------------------------------------------------------------------------
    // PlugIn for App
    
    class AOIPlugin : public Draw::PlugIn {
    public:
        // type for a flock: an STL vector of Boid pointers
        typedef std::vector<AOIUnit*> groupType;
        typedef groupType::const_iterator iterator;
        
        const char* name (void) {return "AOI";}
        
        float selectionOrderSortKey (void) {return 0.01f;}
        
        virtual ~AOIPlugin() {} // be more "nice" to avoid a compiler warning
        
        virtual void open (void) {
            population = 0;
            ipos pos = {.x=-256,.y=-256};
            isize size ={.w=512, .h=512};
            aoimap = imapmake(&pos, &size, 10);
            for (int i = 0; i < 8; i++) addUnitToFlock ();
        }
        
        virtual void close (void)  {
            // delete each member of the flock
            while (population > 0) removeUnitFromFlock ();
            // free all the map
            imapfree(aoimap);
        }
        
        virtual void update (const float currentTime, const float elapsedTime) {
            // update flock simulation for each boid
            for (iterator i = flock.begin(); i != flock.end(); i++)
            {
                AOIUnit * unit = *i;
                unit->update(currentTime, elapsedTime);
            }
        }
        
        virtual void redraw (const float currentTime, const float elapsedTime) {
            // selected vehicle (user can mouse click to select another)
            Draw::AbstractVehicle& selected = *Draw::App::selectedVehicle;
            
            // vehicle nearest mouse (to be highlighted)
            Draw::AbstractVehicle& nearMouse = *Draw::App::vehicleNearestToMouse ();
            
            // update camera
            Draw::App::updateCamera (currentTime, elapsedTime, selected);
            
            // XZ
            Draw::drawXZCheckerboardGrid(256, 8, Draw::Vec3(0,0,0), Draw::Color(0.5, 0.1, 0.2), Draw::Color(0.0, 0.1, 0.5));
            
            // draw each boid in flock
            for (iterator i = flock.begin(); i != flock.end(); i++) (**i).draw ();
            
            // highlight vehicle nearest mouse
           // Draw::App::drawCircleHighlightOnVehicle (nearMouse, 1, Draw::gGray70);
            
            // highlight selected vehicle
           // Draw::App::drawCircleHighlightOnVehicle (selected, 1, Draw::gGray50);
        }
        
        // allows a PlugIn to nominate itself as App's initially selected
        // (default) PlugIn, which is otherwise the first in "selection order"
        virtual bool requestInitialSelection (void) {
            return false;
        }
        
        // handle function keys (which are reserved by SterTest for PlugIns)
        virtual void handleFunctionKeys (int keyNumber) {
            if (keyNumber == 1) {
                for (int i=0; i<8; ++i) addUnitToFlock();
            } else if (keyNumber == 2) {
                for (int i=0; i<8; ++i) removeUnitFromFlock();
            } else if (keyNumber == 3) {
                imapstatedesc(aoimap, EnumMapStateAll, NULL, NULL);
            } else if (keyNumber == 4) {
                iaoimemorystate();
            }
        }
        
        // print "mini help" documenting function keys handled by this PlugIn
        virtual void printMiniHelpForFunctionKeys (void) {
            std::ostringstream message;
            message << "Function keys handled by ";
            message << '"' << name() << '"' << ':' << std::ends;
            Draw::App::printMessage (message);
            Draw::App::printMessage ("  F1     add a boid to the flock.");
            Draw::App::printMessage ("  F2     remove a boid from the flock.");
            Draw::App::printMessage ("  F3     use next proximity database.");
            Draw::App::printMessage ("  F4     next flock boundary condition.");
            Draw::App::printMessage ("");
        }
        
        void addUnitToFlock (void)
        {
            population++;
            AOIUnit* boid = new AOIUnit ();
            flock.push_back (boid);
            if (population == 1) Draw::App::selectedVehicle = boid;
            if (boid->aoiunit->id == 0) {
                boid->setPosition(Vec3(0, 1.0, 0));
                boid->setSpeed(0);
            }
        }
        
        void removeUnitFromFlock (void)
        {
            if (population > 0)
            {
                // save a pointer to the last boid, then remove it from the flock
                const AOIUnit* boid = flock.back();
                flock.pop_back();
                population--;
                
                // if it is OpenSteerDemo's selected vehicle, unselect it
                if (boid == Draw::App::selectedVehicle)
                    Draw::App::selectedVehicle = NULL;
                
                // delete the Boid
                delete boid;
            }
        }
        
        // return an AVGroup (an STL vector of AbstractVehicle pointers) of
        // all vehicles(/agents/characters) defined by the PlugIn
        virtual const Draw::AVGroup& allVehicles (void) {
            return (Draw::AVGroup&)(flock);
        }
        groupType flock;
        int population;
    };
    
    AOIPlugin gAOIPlugIn;

    // ----------------------------------------------------------------------------

} // anonymous namespace
