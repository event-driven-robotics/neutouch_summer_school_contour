// Standard libraries
#include <unistd.h>
#include <cmath>
#include <string>
#include <cstdlib>
#include <vector>
#include <iostream>

// Yarp libraries
#include <yarp/os/all.h>
#include <yarp/math/Math.h>
#include <yarp/sig/Vector.h>
#include <yarp/sig/Matrix.h>
#include <yarp/dev/Drivers.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/dev/IEncoders.h>
#include <yarp/dev/IPositionControl.h>
#include <yarp/dev/CartesianControl.h>

// iCub libraries
#include <iCub/iKin/iKinFwd.h>
#include <iCub/skinDynLib/skinContact.h>
#include <iCub/skinDynLib/skinContactList.h>

using namespace yarp::os;
using namespace yarp::dev;
using namespace yarp::sig;
using namespace yarp::math;

class ContourFollowingModule : public RFModule
{

private:
    BufferedPort<iCub::skinDynLib::skinContactList> skinEventsPort;
    BufferedPort<yarp::sig::Vector> taxelsOutputPort;
    // Cartesian control interfaces
    PolyDriver cartDriver;
    ICartesianControl *cartControl;
    IControlMode *cartControlMode;

    // Position control interfaces
    PolyDriver drvTorso;
    PolyDriver drvRightArm;
    PolyDriver drvLeftArm;
    PolyDriver drvHead;
    IPositionControl *posControlRightArm;
    IPositionControl *posControlLeftArm;
    IPositionControl *posControlHead;
    IPositionControl *posControlTorso;
    IControlMode *controlModeRightArm;
    IControlMode *controlModeLeftArm;
    IControlMode *controlModeHead;
    IControlMode *controlModeTorso;
    IEncoders *iencs;
    RpcServer rpcServer;

    static constexpr double wait_ping{.1};
    static constexpr double wait_tmo{3.};
    std::string robot, arm;
    int startup_context_id_arm;
    double period;

    // Default orientation
    Vector orientation_0;
private:

    void closeHand(IPositionControl *posControl)
    {
        posControl->setRefSpeeds(std::vector<double>(15, 30).data());
        std::vector<int> fingerjoints = {7, 11, 12, 13, 14, 15};
        std::vector<double> fingerClosedPos = {60, 0, 2, 82, 140, 230};
        posControl->positionMove(fingerjoints.size(), fingerjoints.data(), fingerClosedPos.data());

        bool done = false;
        double now = Time::now();

        while (!done && Time::now() - now < wait_tmo)
        {
            sleep(1);
            posControl->checkMotionDone(&done);
        }

        std::vector<int> thumbjoints = {8, 9, 10};
        std::vector<double> thumbClosedPos = {0, 0, 0};
        posControl->positionMove(thumbjoints.size(), thumbjoints.data(), thumbClosedPos.data());

        now = Time::now();
        done = false;
        while (!done && Time::now() - now < wait_tmo)
        {
            sleep(1);
            posControl->checkMotionDone(&done);
        }
    }

    void parseParams(const ResourceFinder &rf)
    {
        setName((rf.check("name", Value("/contour_following")).asString()).c_str());
        robot = rf.check("robot", Value("icubSim")).asString();
        arm = rf.check("arm", Value("right_arm")).asString();
        period = rf.check("period", Value(0.01)).asDouble();
    }

    bool openDrivers()
    {
        // Setting up device for moving the arm in cartesian space
        Property options_arm;
        options_arm.put("device", "cartesiancontrollerclient");
        options_arm.put("remote", "/" + robot + "/cartesianController/" + arm);
        options_arm.put("local", getName() + "/" + arm);
        if (!cartDriver.open(options_arm))
        {
            return false;
        }

        cartDriver.view(cartControl);

        Property optionsRightArm;
        optionsRightArm.put("device", "remote_controlboard");
        optionsRightArm.put("local", getName() + "/controlboard/right_arm");
        optionsRightArm.put("remote", "/" + robot + "/right_arm");
        if (drvRightArm.open(optionsRightArm))
        {
            if (!drvRightArm.view(posControlRightArm) || !drvRightArm.view(controlModeRightArm))
            {
                std::cout << "Could not view driver" << std::endl;
                return false;
            }
        }
        else
        {
            std::cout << "Could not open remote controlboard" << std::endl;
            return false;
        }

        Property optionsLeftArm;
        optionsLeftArm.put("device", "remote_controlboard");
        optionsLeftArm.put("local", getName() + "/controlboard/left_arm");
        optionsLeftArm.put("remote", "/" + robot + "/left_arm");
        if (drvLeftArm.open(optionsLeftArm))
        {
            if (!drvLeftArm.view(posControlLeftArm) || !drvLeftArm.view(controlModeLeftArm))
            {
                std::cout << "Could not view driver" << std::endl;
                return false;
            }
        }
        else
        {
            std::cout << "Could not open remote controlboard" << std::endl;
            return false;
        }

        Property optionsHead;
        optionsHead.put("device", "remote_controlboard");
        optionsHead.put("local", getName() + "/controlboard/head");
        optionsHead.put("remote", "/" + robot + "/head");
        if (drvHead.open(optionsHead))
        {
            if (!drvHead.view(posControlHead) || !drvHead.view(controlModeHead))
            {
                std::cout << "Could not view driver" << std::endl;
                return false;
            }
        }
        else
        {
            std::cout << "Could not open remote controlboard" << std::endl;
            return false;
        }

        Property optionsTorso;
        optionsTorso.put("device", "remote_controlboard");
        optionsTorso.put("local", getName() + "/controlboard/torso");
        optionsTorso.put("remote", "/" + robot + "/torso");
        if (drvTorso.open(optionsTorso))
        {
            if (!drvTorso.view(posControlTorso) || !drvTorso.view(controlModeTorso))
            {
                std::cout << "Could not view driver" << std::endl;
                return false;
            }

        }
        else
        {
            std::cout << "Could not open remote controlboard" << std::endl;
            return false;
        }

        if (arm == "right_arm")
        {
            drvRightArm.view(iencs);
            drvRightArm.view(cartControlMode);
        }
        else if (arm == "left_arm")
        {
            drvLeftArm.view(iencs);
            drvLeftArm.view(cartControlMode);
        }
        else
        {
            std::cout << "Sorry i only have a left and a right arm" << std::endl;
            return false;
        }
        return true;
    }

    bool moveEndEffectorToFingertip()
    {
        // We are interested in the pose of the index when it is totally open,
        // hence it suffices to provide a 9-dimensional vector of zero vectors
        // except for the first angle indicating the iCub adduction/abduction
        // that need to be 60.0 degrees
        Vector fake_encs(9);
        fake_encs.zero();
        fake_encs[0] = 60.0 * M_PI / 180.0;

        Vector joints;
        iCub::iKin::iCubFinger finger("right_index"); // relevant code to get the position of the finger tip
        Matrix tipFrame = finger.getH(fake_encs);

        Vector tip_x = tipFrame.getCol(3);

        // To simplify the development, we will only move the end effector position to the fingertip
        // while leaving the orientation of the hand palm
        Vector identity_axis_angle(4);
        identity_axis_angle.zero();
        // Even if the angle is zero, we should provide a suitable direction for the axis
        identity_axis_angle[0] = 1.0;

        // Establish the new controlled frame
        cartControl->attachTipFrame(tip_x, identity_axis_angle);
        return true;
    }

    void home()
    {
        controlModeRightArm->setControlModes(std::vector<int>(15, VOCAB_CM_POSITION).data());
        controlModeLeftArm->setControlModes(std::vector<int>(15, VOCAB_CM_POSITION).data());
        controlModeHead->setControlModes(std::vector<int>(6, VOCAB_CM_POSITION).data());
        controlModeTorso->setControlModes(std::vector<int>(3, VOCAB_CM_POSITION).data());
        posControlRightArm->setRefSpeeds(std::vector<double>(15, 30).data());
        posControlLeftArm->setRefSpeeds(std::vector<double>(15, 30).data());
        posControlHead->setRefSpeeds(std::vector<double>(6, 30).data());
        posControlTorso->setRefSpeeds(std::vector<double>(3, 15).data());
        posControlRightArm->positionMove(std::vector<double>{-30, 30, 0, 45, 0, 0, 0, 60, 0, 0, 0, 0, 0, 0, 0, 0}.data());
        posControlLeftArm->positionMove(std::vector<double>{-30, 30, 0, 45, 0, 0, 0, 60, 0, 0, 0, 0, 0, 0, 0, 0}.data());
        posControlHead->positionMove(std::vector<double>{0, 0, 0, 0, 0, 0}.data());
        posControlTorso->positionMove(std::vector<double>{0, 0, 0}.data());
        bool done = false;
        double now = Time::now();
        while (!done && Time::now() - now < wait_tmo)
        {
            done = true;
            bool check;
            posControlLeftArm->checkMotionDone(&check);
            done &= check;
            posControlRightArm->checkMotionDone(&check);
            done &= check;
            posControlHead->checkMotionDone(&check);
            done &= check;
            posControlTorso->checkMotionDone(&check);
            done &= check;
            sleep(1);
        }
    }

public:

    bool configure(ResourceFinder &rf)
    {

        parseParams(rf);

        // Open and connect skinEventsPort for reading skin events
        const char *skinEventsPortName = "/skin_events:i";
        skinEventsPort.open(skinEventsPortName);
        if (!skinEventsPort.open(skinEventsPortName))
        {
            yError() << "Port could not open";
            return false;
        }
        if (!rpcServer.open(getName("/rpc")))
        {
            yError() << "Could not open rpc port";
            return false;
        }
        Network::connect("/right_hand/skinManager/skin_events:o", skinEventsPortName);

        // Open output port for taxels pressure debugging
        if (!taxelsOutputPort.open("/taxels_output:o"))
        {
            yError() << "Could not taxels output port";
            return false;
        }

        // Initialize control boards
        if (!openDrivers()) return false;
        home();
        if (arm == "right_arm")
        {
            controlModeRightArm->setControlModes(std::vector<int>(15, VOCAB_CM_POSITION).data());
            closeHand(posControlRightArm);
        }
        else
        {
            controlModeLeftArm->setControlModes(std::vector<int>(15, VOCAB_CM_POSITION).data());
            closeHand(posControlLeftArm);
        }

        //Storing initial context for restoring it later
        cartControl->storeContext(&startup_context_id_arm);
        // Note: the trajectory time only represents the responsiveness of the controller,
        // it does not correspond to the time required to execute the trajectory
        cartControl->setTrajTime(1.0);

        // Enable torso pitch and yaw only
        yarp::sig::Vector dof;
        cartControl->getDOF(dof);
        dof[0]=1;    // torso pitch: 1 => enable
        dof[1]=0;    // torso roll:  2 => disable
        dof[2]=1;    // torso yaw:   1 => enable
        cartControl->setDOF(dof, dof);

        // Move the end effector from the hand palm to the right hand index fingertip
        if (!moveEndEffectorToFingertip())
          return false;

        // Attach RPC server
        attach(rpcServer);

        //--- Move to starting pose ---//

        // Please refer to link below for detailed description of robot frames
        // https://icub-tech-iit.github.io/documentation/icub_kinematics/icub-forward-kinematics/icub-forward-kinematics
        yarp::sig::Vector x0{-0.3, 0.0, 0.0}; // Starting end effector position (0)
        yarp::sig::Vector x1{-0.4, 0.13, -0.1}; // Starting end effector position (1)

        //Rotation from root frame to end effector pointing straight ahead
        Matrix R = zeros(3, 3);
        R(0, 0) = -1.;
        R(1, 1) = 1.;
        R(2, 2) = -1.;

        // Transformation to tilt a bit the hand and avoid the fingers touching the surface
        Matrix tiltAngle = euler2dcm(Vector{0, -35.0 * M_PI / 180.0, 0});

        // Evaluate the overall orientation
        orientation_0 = dcm2axis(tiltAngle * R);

        cartControl->goToPoseSync(x0, orientation_0, 3.0);
        cartControl->waitMotionDone(wait_ping, wait_tmo);
        cartControl->goToPoseSync(x1, orientation_0, 3.0);
        cartControl->waitMotionDone(wait_ping, wait_tmo);

        return true;
    }

    bool respond(const Bottle &command, Bottle &reply)
    {
        if (command.get(0).asString() == "home")
        {
            reply.addString("Going back home");
            home();
        }
        else
        {
            reply.addString("Command not recognized. Available commands: home");
        }

        return true;
    }

    // Send taxels pressure to output port
    void sendTaxelsOutput(iCub::skinDynLib::skinContactList* list)
    {
        yarp::sig::Vector& output = taxelsOutputPort.prepare();
        output.resize(12);
        output.zero();

        if (list != nullptr)
        {
            for (const auto& contact : *list)
            {
                const int id = contact.getTaxelList()[0];
                if (id < 12)
                    output[id] = contact.getPressure();
            }
        }

        taxelsOutputPort.write();
    }

    // Synchronous update every getPeriod() seconds
    bool updateModule()
    {

        //Reading skin events
        iCub::skinDynLib::skinContactList *input = skinEventsPort.read(false);

        // Please do not remove this line
        sendTaxelsOutput(input);

        if (input != nullptr)
        {
            // PUT YOUR CODE HERE

            // See instructions at https://github.com/event-driven-robotics/neutouch_summer_school_contour#useful-tips
        }

        return true;
    }

    double getPeriod()
    {
        return period;
    }

    bool interruptModule()
    {
        yInfo() << "Interrupting module ...";
        skinEventsPort.interrupt();
        return RFModule::interruptModule();
    }

    bool close()
    {
        yInfo() << "Closing the module...";
        skinEventsPort.close();
        cartControl->stopControl();
        cartControl->restoreContext(startup_context_id_arm);
        cartDriver.close();

        yInfo() << "...done!";
        return RFModule::close();
    }
};


int main(int argc, char *argv[])
{
    // Connecting to the yarp server (must be running already)
    Network yarp;
    if (!yarp.checkNetwork(2.0)) {
        yError() << "Network not found";
        return -1;
    }

    // Configuring and running the module
    ContourFollowingModule jCtrl;
    ResourceFinder rf;
    rf.configure(argc, argv);

    return jCtrl.runModule(rf);
}
