#pragma once

/*
+ + +          NAPLPS for openFrameworks       + + +
+ + +     (part of the TelidonP5 Project)      + + +
+ + +   Nick Fox-Gieg  https://fox-gieg.com    + + +

A port of js/telidon/TelidonP5.js: the drawing half of the library, where
decoded NapCmds finally become pixels. All the openFrameworks-specific
code lives here, the same way the p5.js-specific code lived in TelidonP5.js.

Differences from the JavaScript original:
* p5's global fill/stroke state is a real object here, TelidonRenderState,
  shared by every command in a drawing. NAPLPS relies on that state carrying
  over between commands: a SET COLOR command turns fill back on, and an
  outlined shape turns it back off for everything that follows it.
* The scanline fields (moveScanline, scanPos, scanDelta) are gone. They were
  switched off and their pixel loop was commented out in the JS version.
* update() and draw() are separate, the way an ofApp expects. In the JS
  version, run() did both at once for each command in turn.
*/

#include "ofMain.h"

#include "Naplps.h"

// The p5.js drawing state that carries over from one command to the next.
class TelidonRenderState {

    public:

        TelidonRenderState();

        void reset();

        ofColor color;
        bool doFill;
        bool doStroke;
        float strokeWeight;

};

class TelidonDrawCmd {

    public:

        TelidonDrawCmd(const NapCmd & _cmd, float _w, float _h, std::shared_ptr<TelidonRenderState> _renderState);

        void update();

        // *** IMPORTANT STEP 3 of 3 ***
        // This is where the decoded commands finally get drawn to the screen.
        void draw();

        void run() { update(); draw(); }

        void reset();

        void setSize(float _w, float _h) { w = _w; h = _h; }

        NapCmd cmd;
        float w;
        float h;

        bool progressiveDraw;
        bool labelPoints;
        float thickness;
        uint64_t progressiveDrawInterval;

        std::vector<glm::vec2> points;
        int pointsIndex;
        bool finished;

    private:

        void setColor(const ofColor & v);
        void drawText(const std::string & text);
        void drawRect(const std::vector<glm::vec2> & pts, float w, float h, bool isFill);
        void drawArc(const std::vector<glm::vec2> & pts, float w, float h, bool isFill);
        void drawPoints(const std::vector<glm::vec2> & pts, float w, float h, bool isFill = false);

        // matches p5's beginShape() / vertex() / endShape(CLOSE)
        ofPath makeShape(const std::vector<glm::vec2> & pts, float w, float h) const;
        void styleShape(ofPath & path) const;

        std::shared_ptr<TelidonRenderState> renderState;

        uint64_t markTime;
        int extraLoopCounter;

};

// 5. Drawing class -- this is where it all comes together.
class TelidonDraw {

    public:

        TelidonDraw();

        // Decode a .nap file and get it ready to draw, in one step.
        bool load(const std::string & filePath, float _w = 0, float _h = 0);

        // Or draw a file that's already been decoded.
        void setup(const NapDecoder & _decoder, float _w = 0, float _h = 0);

        void update();
        void draw();

        // Start the progressive drawing over again.
        void reset();

        // The unit screen is scaled to w by h. Both are the window size by default.
        void setSize(float _w, float _h);

        // Draw everything at once instead of one command at a time.
        void setProgressiveDraw(bool _progressiveDraw);

        // Milliseconds between points while progressively drawing.
        void setProgressiveDrawInterval(uint64_t _interval);

        void setThickness(float _thickness);
        void setLabelPoints(bool _labelPoints);

        bool isFinished() const { return finished; }
        bool isLoaded() const { return !drawCmds.empty(); }

        NapDecoder decoder;
        std::vector<TelidonDrawCmd> drawCmds;
        bool finished;

        float w;
        float h;

    private:

        std::shared_ptr<TelidonRenderState> renderState;

        bool progressiveDraw;
        uint64_t progressiveDrawInterval;
        float thickness;
        bool labelPoints;

};

// The renderer is the other front door of the library, so it gets a friendly name too.
typedef TelidonDraw Telidon;
