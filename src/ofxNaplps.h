#pragma once

/*
+ + +   ofxNaplps: read and write NAPLPS/Telidon files in openFrameworks   + + +
+ + +   Nick Fox-Gieg  https://fox-gieg.com                                + + +

    Naplps naplps;   // the decoder,  ported from naplps.js
    Telidon telidon; // the renderer, ported from TelidonP5.js

    naplps.load("shark.nap");
    telidon.setup(naplps, size, size);

    ...then telidon.update() and telidon.draw() from your ofApp.
*/

#include "ofxNaplps/Naplps.h"
#include "ofxNaplps/Telidon.h"
