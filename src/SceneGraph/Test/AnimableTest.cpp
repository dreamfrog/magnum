/*
    Copyright © 2010, 2011, 2012 Vladimír Vondruš <mosra@centrum.cz>

    This file is part of Magnum.

    Magnum is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License version 3
    only, as published by the Free Software Foundation.

    Magnum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License version 3 for more details.
*/

#include "AnimableTest.h"

#include <sstream>

#include "SceneGraph/Animable.h"
#include "SceneGraph/AnimableGroup.h"
#include "SceneGraph/MatrixTransformation3D.h"

CORRADE_TEST_MAIN(Magnum::SceneGraph::Test::AnimableTest)

namespace Magnum { namespace SceneGraph { namespace Test {

typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D<>> Object3D;

AnimableTest::AnimableTest() {
    addTests(&AnimableTest::state,
             &AnimableTest::step,
             &AnimableTest::duration,
             &AnimableTest::repeat,
             &AnimableTest::stop,
             &AnimableTest::pause,
             &AnimableTest::debug);
}

void AnimableTest::state() {
    class StateTrackingAnimable: public SceneGraph::Animable<3> {
        public:
            StateTrackingAnimable(AbstractObject<3>* object, AnimableGroup<3>* group = nullptr): SceneGraph::Animable<3>(object, 1.0f, group) {}

            std::string trackedState;

        protected:
            void animationStep(GLfloat, GLfloat) override {}

            void animationStarted() override { trackedState += "started"; }
            void animationPaused() override { trackedState += "paused"; }
            void animationResumed() override { trackedState += "resumed"; }
            void animationStopped() override { trackedState += "stopped"; }
    };

    Object3D object;
    AnimableGroup<3> group;
    CORRADE_COMPARE(group.runningCount(), 0);

    /* Verify initial state */
    StateTrackingAnimable animable(&object, &group);
    CORRADE_COMPARE(animable.state(), AnimationState::Stopped);
    CORRADE_VERIFY(animable.trackedState.empty());
    group.step(1.0f, 1.0f);
    CORRADE_VERIFY(animable.trackedState.empty());
    CORRADE_COMPARE(group.runningCount(), 0);

    /* Stopped -> paused is not supported */
    CORRADE_COMPARE(animable.state(), AnimationState::Stopped);
    animable.setState(AnimationState::Paused);
    CORRADE_COMPARE(animable.state(), AnimationState::Stopped);

    /* Stopped -> running */
    CORRADE_COMPARE(animable.state(), AnimationState::Stopped);
    animable.trackedState.clear();
    animable.setState(AnimationState::Running);
    CORRADE_VERIFY(animable.trackedState.empty());
    group.step(1.0f, 1.0f);
    CORRADE_COMPARE(animable.trackedState, "started");
    CORRADE_COMPARE(group.runningCount(), 1);

    /* Running -> paused */
    CORRADE_COMPARE(animable.state(), AnimationState::Running);
    animable.trackedState.clear();
    animable.setState(AnimationState::Paused);
    CORRADE_VERIFY(animable.trackedState.empty());
    group.step(1.0f, 1.0f);
    CORRADE_COMPARE(animable.trackedState, "paused");
    CORRADE_COMPARE(group.runningCount(), 0);

    /* Paused -> running */
    CORRADE_COMPARE(animable.state(), AnimationState::Paused);
    animable.trackedState.clear();
    animable.setState(AnimationState::Running);
    CORRADE_VERIFY(animable.trackedState.empty());
    group.step(1.0f, 1.0f);
    CORRADE_COMPARE(animable.trackedState, "resumed");
    CORRADE_COMPARE(group.runningCount(), 1);

    /* Running -> stopped */
    CORRADE_COMPARE(animable.state(), AnimationState::Running);
    animable.trackedState.clear();
    animable.setState(AnimationState::Stopped);
    CORRADE_VERIFY(animable.trackedState.empty());
    group.step(1.0f, 1.0f);
    CORRADE_COMPARE(animable.trackedState, "stopped");
    CORRADE_COMPARE(group.runningCount(), 0);

    animable.setState(AnimationState::Running);
    group.step(1.0f, 1.0f);
    animable.setState(AnimationState::Paused);

    /* Paused -> stopped */
    CORRADE_COMPARE(animable.state(), AnimationState::Paused);
    animable.trackedState.clear();
    animable.setState(AnimationState::Stopped);
    CORRADE_VERIFY(animable.trackedState.empty());
    group.step(1.0f, 1.0f);
    CORRADE_COMPARE(animable.trackedState, "stopped");
    CORRADE_COMPARE(group.runningCount(), 0);

    /* Verify running count can go past 0/1 */
    group.add((new StateTrackingAnimable(&object, &group))->setState(AnimationState::Running));
    group.add((new StateTrackingAnimable(&object, &group))->setState(AnimationState::Running));
    group.step(1.0f, 1.0f);
    CORRADE_COMPARE(group.runningCount(), 2);
}

class OneShotAnimable: public SceneGraph::Animable<3> {
    public:
        OneShotAnimable(AbstractObject<3>* object, AnimableGroup<3>* group = nullptr): SceneGraph::Animable<3>(object, 10.0f, group), time(-1.0f) {
            setState(AnimationState::Running);
        }

        GLfloat time;

    protected:
        void animationStep(GLfloat time, GLfloat) override {
            this->time = time;
        }
};

void AnimableTest::step() {
    class InifiniteAnimable: public SceneGraph::Animable<3> {
        public:
            InifiniteAnimable(AbstractObject<3>* object, AnimableGroup<3>* group = nullptr): SceneGraph::Animable<3>(object, 0.0f, group), time(-1.0f), delta(0.0f) {}

            GLfloat time, delta;

        protected:
            void animationStep(GLfloat time, GLfloat delta) override {
                this->time = time;
                this->delta = delta;
            }
    };

    Object3D object;
    AnimableGroup<3> group;
    InifiniteAnimable animable(&object, &group);

    /* Calling step() if no object is running should do nothing */
    group.step(5.0f, 0.5f);
    CORRADE_COMPARE(group.runningCount(), 0);
    CORRADE_COMPARE(animable.time, -1.0f);
    CORRADE_COMPARE(animable.delta, 0.0f);

    /* Calling step() with running animation should start it with zero
       absolute time */
    animable.setState(AnimationState::Running);
    group.step(5.0f, 0.5f);
    CORRADE_COMPARE(group.runningCount(), 1);
    CORRADE_COMPARE(animable.time, 0.0f);
    CORRADE_COMPARE(animable.delta, 0.5f);

    /* Repeated call to step() will add to absolute animation time */
    group.step(8.0f, 0.75f);
    CORRADE_COMPARE(animable.time, 3.0f);
    CORRADE_COMPARE(animable.delta, 0.75f);
}

void AnimableTest::duration() {
    Object3D object;
    AnimableGroup<3> group;
    OneShotAnimable animable(&object, &group);
    CORRADE_VERIFY(!animable.isRepeated());

    /* First animation step is in duration, verify that animation is still
       running and animationStep() is called */
    group.step(1.0f, 0.5f);
    CORRADE_COMPARE(animable.state(), AnimationState::Running);
    CORRADE_COMPARE(animable.time, 0.0f);

    /* Next animation step is out of duration and repeat is not enabled,
       animationStep() shouldn't be called and animation should be stopped */
    group.step(12.75f, 0.5f);
    CORRADE_COMPARE(animable.state(), AnimationState::Stopped);
    CORRADE_COMPARE(animable.time, 0.0f);
}

void AnimableTest::repeat() {
    class RepeatingAnimable: public SceneGraph::Animable<3> {
        public:
            RepeatingAnimable(AbstractObject<3>* object, AnimableGroup<3>* group = nullptr): SceneGraph::Animable<3>(object, 10.0f, group), time(-1.0f) {
                setState(AnimationState::Running);
                setRepeated(true);
            }

            GLfloat time;

        protected:
            void animationStep(GLfloat time, GLfloat) override {
                this->time = time;
            }
    };

    Object3D object;
    AnimableGroup<3> group;
    RepeatingAnimable animable(&object, &group);
    CORRADE_COMPARE(animable.repeatCount(), 0);

    /* First animation steps is in first loop iteration */
    group.step(1.0f, 0.5f);
    CORRADE_COMPARE(animable.state(), AnimationState::Running);
    CORRADE_COMPARE(animable.time, 0.0f);

    /* Next animation step is in second loop iteration, animation should be
       still running with time shifted by animation duration */
    group.step(11.5f, 0.5f);
    CORRADE_COMPARE(animable.state(), AnimationState::Running);
    CORRADE_COMPARE(animable.time, 0.5f);

    /* Third loop iteration (just to be sure) */
    group.step(25.5f, 0.5f);
    CORRADE_COMPARE(animable.state(), AnimationState::Running);
    CORRADE_COMPARE(animable.time, 4.5f);

    /* Cap repeat count to 3, the animation should be stopped now (and
       animationStep() shouldn't be called)*/
    animable.setRepeatCount(3);
    group.step(33.0f, 0.5f);
    CORRADE_COMPARE(animable.state(), AnimationState::Stopped);
    CORRADE_COMPARE(animable.time, 4.5f);
}

void AnimableTest::stop() {
    Object3D object;
    AnimableGroup<3> group;
    OneShotAnimable animable(&object, &group);
    CORRADE_COMPARE(animable.repeatCount(), 0);

    /* Eat up some absolute time */
    group.step(1.0f, 0.5f);
    group.step(1.5f, 0.5f);
    CORRADE_COMPARE(animable.state(), AnimationState::Running);
    CORRADE_COMPARE(animable.time, 0.5f);

    /* Stop the animable, nothing should be done */
    animable.setState(AnimationState::Stopped);
    group.step(1.5f, 0.5f);
    CORRADE_COMPARE(animable.state(), AnimationState::Stopped);
    CORRADE_COMPARE(animable.time, 0.5f);

    /* Restarting the animation should start with zero absolute time */
    animable.setState(AnimationState::Running);
    group.step(2.5f, 0.5f);
    CORRADE_COMPARE(animable.state(), AnimationState::Running);
    CORRADE_COMPARE(animable.time, 0.0f);
}

void AnimableTest::pause() {
    Object3D object;
    AnimableGroup<3> group;
    OneShotAnimable animable(&object, &group);

    /* First two steps, animation is running */
    group.step(1.0f, 0.5f);
    group.step(2.5f, 0.5f);
    CORRADE_COMPARE(animable.state(), AnimationState::Running);
    CORRADE_COMPARE(animable.time, 1.5f);

    /* Pausing the animation, first step should decrease count of running
       animations and save paused time, next steps shouldn't affect anything */
    CORRADE_COMPARE(group.runningCount(), 1);
    animable.setState(AnimationState::Paused);
    CORRADE_COMPARE(group.runningCount(), 1);
    group.step(3.0f, 0.5f);
    CORRADE_COMPARE(group.runningCount(), 0);
    group.step(4.5f, 0.5f);
    CORRADE_COMPARE(animable.state(), AnimationState::Paused);
    CORRADE_COMPARE(animable.time, 1.5f);

    /* Unpausing, next step should continue from absolute time when pause
       occured */
    animable.setState(AnimationState::Running);
    group.step(5.0f, 0.5f);
    CORRADE_COMPARE(animable.state(), AnimationState::Running);
    CORRADE_COMPARE(animable.time, 2.0f);
}

void AnimableTest::debug() {
    std::ostringstream o;
    Debug(&o) << AnimationState::Running;
    CORRADE_COMPARE(o.str(), "SceneGraph::AnimationState::Running\n");
}

}}}