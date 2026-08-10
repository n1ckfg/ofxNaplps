# ofxNaplps
<img src="ofxaddons_thumbnail.png"><br>
Read and write Naplps files in openFrameworks, tested with oF 0.12.1.<br> 
Available from https://ofxaddons.com

A port of the [Telidon](https://github.com/n1ckfg/Telidon) JavaScript library:
`naplps.js` became `Naplps.cpp` (the decoder and encoder) and `TelidonP5.js`
became `Telidon.cpp` (the renderer).

## Reading

```cpp
#include "ofxNaplps.h"

Naplps naplps;   // decoder
Telidon telidon; // renderer

void ofApp::setup() {
    naplps.load("shark.nap");
    telidon.setup(naplps, ofGetHeight(), ofGetHeight());
}

void ofApp::update() {
    telidon.update();
}

void ofApp::draw() {
    ofBackground(0);
    telidon.draw();
}
```

Points live on the NAPLPS unit screen, from (0,0) to (1,1), so pass `Telidon`
a square if you don't want the drawing stretched. By default it draws
progressively, one point every 66 milliseconds, the way a Telidon terminal
did; `telidon.setProgressiveDraw(false)` draws the whole thing at once and
`telidon.reset()` starts it over.

The decoded commands are available directly, if you'd rather draw them yourself:

```cpp
for (NapCmd & cmd : naplps.cmds) {
    ofLogNotice() << cmd.formatCmd("hex");
    // cmd.opcode.opId, cmd.points, cmd.col, cmd.text
}
```

## Writing

```cpp
NapEncoder encoder;

std::vector<NapInputWrapper> strokes;
strokes.push_back(NapInputWrapper(ofColor::red, points, false)); // color, points, isFill

encoder.encode(strokes, 4); // 4 bytes per coordinate
encoder.save("out.nap");
```

## Example

`example/` decodes `bin/data/shark.nap` and draws it. Arrow keys cycle through
the other sample files, space redraws, and you can drop a `.nap` file on the
window.
