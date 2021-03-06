#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>

#include "Leap.h"
#include "main.h"
#include "HandSignal.h"

using namespace Leap;
using namespace std;



HandSignal::HandSignal(const Hand &hand, sensitivity_t config) : HandSignal(hand) { settings = config; }

HandSignal::HandSignal(const Hand &hand) {
    if(DEBUG > 2) cout << "[HandSignal] handSignal ctor called!" << std::endl;
    if (!hand.isValid()) {
        if(DEBUG > 0) cout << "[HandSignal] ERROR: no Hands passed" << endl;
        fingers = 0;
        return;
    }
    if (hand.fingers().count() > 20) {
        fingers = 0;
        return;
    }

    fingers = hand.fingers().count();
    if(DEBUG > 2) cout << "[HandSignals] data received:\n" << hand.fingers() << endl;

    for (int i = 0; i < fingers; i++) {
        fingerLengths[i] = 0;
        fingerExtended[i] = false;
        for (int b = 0; b < 4; b++) {
            for (int w = 0; w < 3; w++) {
                boneStarts[i][b][w] = 0;
                boneEnds[i][b][w] = 0;
                boneDirs[i][b][w] = 0;
            }
        }
    }

    FingerList fl = hand.fingers();
    int i = 0;
    for (FingerList::const_iterator fl_iter = fl.begin(); fl_iter != fl.end(); ++fl_iter, i++) {
        const Finger finger = *fl_iter;
        fingerTypes[i] = finger.type();
        fingerExtended[i] = finger.isExtended();
        fingerLengths[i] += finger.length();
        for (int b = 0; b < 4; ++b) {
            Bone::Type boneType = static_cast<Bone::Type>(b);
            Bone bone = finger.bone(boneType);

            for (int w = 0; w < 3; w++) {
                boneStarts[i][b][w] = bone.prevJoint()[w];
                boneEnds[i][b][w] = bone.nextJoint()[w];
                boneDirs[i][b][w] = bone.direction()[w];
            }
        }
    }
    
    float offset[20][3];
    for (i = 0; i < fingers; i++) {
        for (int w = 0; w < 3; w++) {
            offset[i][w] = boneStarts[i][0][w];
        }
    }
    
    for (i = 0; i < fingers; i++) {
        for (int b = 0; b < 4; b++) {
            for (int w = 0; w < 3; w++) {
                boneStarts[i][b][w] -= offset[i][w];
                boneEnds[i][b][w] -= offset[i][w];
            }
        }
    }
}

HandSignal::HandSignal(const HandSignal &other) : fingers(other.fingers), settings(other.settings)
{
  if(DEBUG > 2) cout << "[HandSignal] Copy ctor called!" << endl;
  for(int i = 0; i < 20; i++)
  {
    fingerLengths[i] = other.fingerLengths[i];
    fingerExtended[i] = other.fingerExtended[i];
    for(int b = 0; b < 4; b++)
    {
      for(int w = 0; w < 3; w++)
      {
        boneStarts[i][b][w] = other.boneStarts[i][b][w];
        boneEnds[i][b][w] = other.boneEnds[i][b][w];
        boneDirs[i][b][w] = other.boneDirs[i][b][w];
      }
    }
  }
}

ostream &operator<<(ostream &os, const HandSignal &hs) {
    if (hs.fingers == 0) return os << "Invalid HandSignal";

    for (int i = 0; i < hs.fingers; i++) {
      os << string(4, ' ') <<  "Length: " << hs.fingerLengths[i]
                << "mm" << endl;

      // Get finger bones
      for (int b = 0; b < 4; ++b) {
        os << string(6, ' ') <<  boneNames[static_cast<Bone::Type>(b)]
                  << " bone, start: (" << hs.boneStarts[i][b][0] << ", " << hs.boneStarts[i][b][1] << ", " << hs.boneStarts[i][b][2] << ")"
                  << ", end: (" << hs.boneEnds[i][b][0] << ", " << hs.boneEnds[i][b][1] << ", " << hs.boneEnds[i][b][2] << ")"
                  << ", direction: (" << hs.boneDirs[i][b][0] << ", " << hs.boneDirs[i][b][1] << ", " << hs.boneDirs[i][b][2] << ")" << endl;
      }
    }
    return os;
}

bool HandSignal::isValid() const {
    return fingers > 0;
}

float valueDiff(float base, float val) {
    return abs(val - base);
}

bool allFalse(const bool arr[])
{
  for(int i = 0; i < 20; i++)
  {
    if(arr[i])
      return false;
  }
  return true;
}

bool HandSignal::matchesSignal(const Hand &hand, int &errorcode) const {
    const FingerList curr_fingers = hand.fingers();
    errorcode = 0;
    if (fingers == 0) {
        errorcode = 1;
        return false;
    }
    else if (curr_fingers.count() != fingers) {
        errorcode = 2;
        return false;
    }
    int i = 0;
    //Vector first = hand.palmPosition();
    // Each finger
    for (FingerList::const_iterator fl_iter = curr_fingers.begin(); fl_iter != curr_fingers.end(); ++fl_iter, i++) {
        const Finger finger = *fl_iter;

        if (finger.isExtended() != fingerExtended[i]) { // if the finger extended status is not the same, it's not the same signal
                errorcode = 7;
                return false;
        }
        if (!finger.isExtended() && !allFalse(fingerExtended)) continue; // we don't care about position of non-extended fingers
        // Finger length
        if (valueDiff(fingerLengths[i], finger.length()) > settings.fingerLengthDiff) {
            errorcode = 3;
            return false;
        }
        // Each bone position
        for (int b = 0; b < 4; b++) {
            Bone::Type boneType = static_cast<Bone::Type>(b);
            Bone bone = finger.bone(boneType);
            // XYZ differences
            for (int w = 0; w < 3; w++)
            {
                if (valueDiff(boneStarts[i][b][w], bone.prevJoint()[w] - finger.bone(static_cast<Bone::Type>(0)).prevJoint()[w]) > settings.positionDiff) {
                    errorcode = 4;
                    if(DEBUG > 2) cout << "startDiff = " << boneStarts[i][b][w] - (bone.prevJoint()[w] - finger.bone(static_cast<Bone::Type>(0)).prevJoint()[w]) << " ";
                }
                else if (valueDiff(boneEnds[i][b][w], bone.nextJoint()[w] - finger.bone(static_cast<Bone::Type>(0)).prevJoint()[w]) > settings.positionDiff) {
                    errorcode = 5;
                    if(DEBUG > 2) cout << "endDiff = " << boneEnds[i][b][w] - (bone.nextJoint()[w] - finger.bone(static_cast<Bone::Type>(0)).prevJoint()[w]) << " > " << settings.positionDiff << " and new bone pos is " << bone.nextJoint()[w] - finger.bone(static_cast<Bone::Type>(0)).prevJoint()[w] << " and trained position is " << boneEnds[i][b][w] << " ";
                }
                else if (valueDiff(boneDirs[i][b][w], bone.direction()[w]) > settings.directionDiff) {
                    errorcode = 6;
                }

                if(errorcode != 0)
                {
                  if(DEBUG > 2) cout << fingerNames[i];
                  return false;
                }
            }
        }
    }
    return true;
}
