#include "ofxNaplps/Telidon.h"

// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
// The drawing state that p5.js kept in globals
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

TelidonRenderState::TelidonRenderState() {
    reset();
}

void TelidonRenderState::reset() {
    color = ofColor(255, 255, 255);
    doFill = true;
    doStroke = true;
    strokeWeight = 1.0f;
}

// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
// One decoded command, drawn
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

TelidonDrawCmd::TelidonDrawCmd(const NapCmd & _cmd, float _w, float _h, std::shared_ptr<TelidonRenderState> _renderState) {
    cmd = _cmd;
    w = _w;
    h = _h;
    renderState = _renderState;

    progressiveDraw = true;
    labelPoints = false;
    thickness = 1.0f;
    progressiveDrawInterval = 66;

    pointsIndex = 0;
    markTime = 0;
    extraLoopCounter = 0;
    finished = false;

    reset();
}

void TelidonDrawCmd::reset() {
    points.clear();
    pointsIndex = 0;
    extraLoopCounter = 0;
    finished = false;
    markTime = ofGetElapsedTimeMillis();

    if (!progressiveDraw) {
        for (int i = 0; i < (int)cmd.points.size(); i++) {
            const glm::vec2 & point = cmd.points[i];
            if (point.x >= 0 && point.x <= 1 && point.y >= 0 && point.y <= 1) {
                points.push_back(point);
            }
        }
        pointsIndex = (int)cmd.points.size();
        markTime = 0;
    }
}

void TelidonDrawCmd::update() {
    uint64_t now = ofGetElapsedTimeMillis();

    if (now > markTime + progressiveDrawInterval) {
        if (progressiveDraw && points.size() < cmd.points.size() && pointsIndex < (int)cmd.points.size()) {
            const glm::vec2 & point = cmd.points[pointsIndex];
            if (point.x >= 0 && point.x <= 1 && point.y >= 0 && point.y <= 1) {
                points.push_back(point);
            }
            pointsIndex++;

            markTime = now;
        }
    }

    // The JS version watches points.length instead of pointsIndex, so a command
    // holding an off-screen point never reports itself finished.
    if (!finished && pointsIndex >= (int)cmd.points.size()) {
        if (extraLoopCounter < (int)progressiveDrawInterval) {
            extraLoopCounter++;
        } else {
            finished = true;
        }
    }
}

void TelidonDrawCmd::draw() {
    // *** IMPORTANT STEP 3 of 3 ***
    // This is where the decoded commands finally get drawn to the screen.
    switch (cmd.opcode.opId) {
        //~ ~ ~ ~ ~ CONTROL CODES ~ ~ ~ ~ ~
        case NAP_OP_SHIFT_OUT: // graphics mode, we're here by default
            // no effect?
            break;
        case NAP_OP_SHIFT_IN: // text mode, data that follows is text
            drawText(cmd.text);
            break;
        case NAP_OP_CANCEL:
            // no effect?
            break;
        case NAP_OP_ESC:
            // no effect?
            break;
        case NAP_OP_NSR: // Non-Selective Reset
            // no effect?
            break;
        //~ ~ ~ ~ ~ PDI (PICTURE DESCRIPTION INSTRUCTION) CODES ~ ~ ~ ~ ~
        //~ ~ ~ ENVIRONMENT, part 1 ~ ~ ~
        case NAP_OP_RESET:
            // TODO
            break;
        case NAP_OP_DOMAIN: // header information
            // TODO
            break;
        case NAP_OP_TEXT:
            // TODO
            break;
        case NAP_OP_TEXTURE:
            // TODO
            break;
        //~ ~ ~ POINTS ~ ~ ~
        case NAP_OP_POINT_SET_ABS:
            drawPoints(points, w, h);
            break;
        case NAP_OP_POINT_SET_REL:
            drawPoints(points, w, h);
            break;
        case NAP_OP_POINT_ABS:
            drawPoints(points, w, h);
            break;
        case NAP_OP_POINT_REL:
            drawPoints(points, w, h);
            break;
        //~ ~ ~ LINES ~ ~ ~
        case NAP_OP_LINE_ABS:
            drawPoints(points, w, h);
            break;
        case NAP_OP_LINE_REL:
            drawPoints(points, w, h);
            break;
        case NAP_OP_SET_LINE_ABS:
            drawPoints(points, w, h);
            break;
        case NAP_OP_SET_LINE_REL:
            drawPoints(points, w, h);
            break;
        //~ ~ ~ ARCS ~ ~ ~
        case NAP_OP_ARC_OUTLINED:
            drawArc(cmd.points, w, h, false);
            break;
        case NAP_OP_ARC_FILLED:
            drawArc(cmd.points, w, h, true);
            break;
        case NAP_OP_SET_ARC_OUTLINED:
            drawArc(cmd.points, w, h, false);
            break;
        case NAP_OP_SET_ARC_FILLED:
            drawArc(cmd.points, w, h, true);
            break;
        //~ ~ ~ RECTANGLES ~ ~ ~
        case NAP_OP_RECT_OUTLINED:
            drawRect(cmd.points, w, h, false);
            break;
        case NAP_OP_RECT_FILLED:
            drawRect(cmd.points, w, h, true);
            break;
        case NAP_OP_SET_RECT_OUTLINED:
            drawRect(cmd.points, w, h, false);
            break;
        case NAP_OP_SET_RECT_FILLED:
            drawRect(cmd.points, w, h, true);
            break;
        //~ ~ ~ POLYGONS ~ ~ ~
        case NAP_OP_POLY_OUTLINED:
            drawPoints(points, w, h, false);
            break;
        case NAP_OP_POLY_FILLED:
            drawPoints(points, w, h, true);
            break;
        case NAP_OP_SET_POLY_OUTLINED: // relative points after first
            drawPoints(points, w, h, false);
            break;
        case NAP_OP_SET_POLY_FILLED: // relative points after first
            drawPoints(points, w, h, true);
            break;
        //~ ~ ~ INCREMENTALS ~ ~ ~
        case NAP_OP_FIELD:
            // TODO
            break;
        case NAP_OP_INCREMENTAL_POINT:
            // TODO
            break;
        case NAP_OP_INCREMENTAL_LINE:
            // TODO
            break;
        case NAP_OP_INCREMENTAL_POLY_FILLED:
            // TODO
            break;
        //~ ~ ~ ENVIRONMENT, part 2 ~ ~ ~
        case NAP_OP_SET_COLOR: // this picks a color
            setColor(cmd.col);
            break;
        case NAP_OP_WAIT:
            // TODO
            break;
        case NAP_OP_SELECT_COLOR: // this sets the color mode
            setColor(cmd.col);
            break;
        case NAP_OP_BLINK:
            // TODO
            break;
        default:
            break;
    }
}

void TelidonDrawCmd::setColor(const ofColor & v) {
    // p5: fill(col); stroke(col); strokeWeight(thickness);
    renderState->color = v;
    renderState->doFill = true;
    renderState->doStroke = true;
    renderState->strokeWeight = thickness; // TODO should this go somewhere else?
}

ofPath TelidonDrawCmd::makeShape(const std::vector<glm::vec2> & pts, float w, float h) const {
    ofPath path;

    for (int i = 0; i < (int)pts.size(); i++) {
        if (i == 0) {
            path.moveTo(pts[i].x * w, pts[i].y * h);
        } else {
            path.lineTo(pts[i].x * w, pts[i].y * h);
        }
    }
    path.close(); // endShape(CLOSE)

    return path;
}

void TelidonDrawCmd::styleShape(ofPath & path) const {
    path.setFilled(renderState->doFill);
    path.setFillColor(renderState->color);
    path.setStrokeColor(renderState->color);
    path.setStrokeWidth(renderState->doStroke ? renderState->strokeWeight : 0.0f);
}

void TelidonDrawCmd::drawText(const std::string & _text) {
    if (_text.length() < 1) return;

    ofPushStyle();
    ofSetColor(renderState->color);
    ofDrawBitmapString(_text, w * 0.0625f, h * 1.25f); // TODO position
    ofPopStyle();
}

void TelidonDrawCmd::drawRect(const std::vector<glm::vec2> & pts, float w, float h, bool isFill) {
    if (!isFill) renderState->doFill = false; // p5: noFill()

    if (pts.size() == 2) {
        float x1 = pts[0].x * w;
        float y1 = pts[0].y * h;
        float x2 = pts[1].x * w;
        float y2 = pts[1].y * h;

        ofPath path;
        path.rectangle(x1, y1, x2 - x1, y2 - y1); // p5: rectMode(CORNER)
        styleShape(path);
        path.draw();
    } else {
        drawPoints(pts, w, h, isFill);
    }
}

void TelidonDrawCmd::drawArc(const std::vector<glm::vec2> & pts, float w, float h, bool isFill) {
    if (!isFill) renderState->doFill = false; // p5: noFill()

    if (pts.size() == 2) {
        float x1 = pts[0].x * w;
        float y1 = pts[0].y * h;
        float x2 = pts[1].x * w;
        //float y2 = pts[1].y * h;
        float d = x2 - x1;

        ofPath path;
        // p5: ellipseMode(CORNER); ellipse(x1, y1, x2-x1, x2-x1);
        path.ellipse(x1 + (d / 2.0f), y1 + (d / 2.0f), d, d);
        styleShape(path);
        path.draw();
    } else {
        ofPath path;
        for (int i = 0; i < (int)pts.size() - 1; i++) {
            float x1 = pts[i].x * w;
            float y1 = pts[i].y * h;
            float x2 = pts[i + 1].x * w;
            float y2 = pts[i + 1].y * h;
            float rx = (x2 - x1) / 2.0f;
            float ry = (y2 - y1) / 2.0f;
            float a1 = (float)i * (180.0f / (float)pts.size());
            float a2 = (float)(i + 1) * (180.0f / (float)pts.size());
            path.arc(x1 + rx, y1 + ry, rx, ry, a1, a2);
        }
        styleShape(path);
        path.draw();
    }
}

void TelidonDrawCmd::drawPoints(const std::vector<glm::vec2> & pts, float w, float h, bool isFill) {
    if (!isFill) renderState->doFill = false; // p5: noFill()

    if (pts.size() > 0) {
        ofPath path = makeShape(pts, w, h);
        styleShape(path);
        path.draw();
    }

    if (labelPoints) {
        // Debug only. Unlike the p5 version this doesn't leave its color behind
        // for the next command to inherit.
        ofPushStyle();
        ofSetColor(255, 63);
        for (int i = 0; i < (int)pts.size(); i++) {
            ofDrawCircle(pts[i].x * w, pts[i].y * h, thickness * 2.0f);
        }
        ofPopStyle();
    }
}

// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
// 5. Drawing class -- this is where it all comes together.
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

TelidonDraw::TelidonDraw() {
    renderState = std::make_shared<TelidonRenderState>();
    finished = false;
    w = 0;
    h = 0;
    progressiveDraw = true;
    progressiveDrawInterval = 66;
    thickness = 1.0f;
    labelPoints = false;
}

bool TelidonDraw::load(const std::string & filePath, float _w, float _h) {
    NapDecoder newDecoder;
    if (!newDecoder.load(filePath)) return false;

    setup(newDecoder, _w, _h);
    return true;
}

void TelidonDraw::setup(const NapDecoder & _decoder, float _w, float _h) {
    decoder = _decoder;

    w = (_w > 0) ? _w : ofGetWidth();
    h = (_h > 0) ? _h : ofGetHeight();

    renderState->reset();
    drawCmds.clear();
    for (int i = 0; i < (int)decoder.cmds.size(); i++) {
        TelidonDrawCmd drawCmd(decoder.cmds[i], w, h, renderState);
        drawCmd.progressiveDraw = progressiveDraw;
        drawCmd.progressiveDrawInterval = progressiveDrawInterval;
        drawCmd.thickness = thickness;
        drawCmd.labelPoints = labelPoints;
        drawCmd.reset();
        drawCmds.push_back(drawCmd);
    }

    finished = drawCmds.empty();
}

void TelidonDraw::update() {
    finished = true;

    for (int i = 0; i < (int)drawCmds.size(); i++) {
        drawCmds[i].update();
        if (!drawCmds[i].finished) finished = false;
    }
}

void TelidonDraw::draw() {
    if (drawCmds.empty()) return;

    if (decoder.version == 699) ofBackground(127);

    ofPushStyle();
    renderState->reset();
    for (int i = 0; i < (int)drawCmds.size(); i++) {
        drawCmds[i].draw();
    }
    ofPopStyle();
}

void TelidonDraw::reset() {
    renderState->reset();
    for (int i = 0; i < (int)drawCmds.size(); i++) {
        drawCmds[i].reset();
    }
    finished = drawCmds.empty();
}

void TelidonDraw::setSize(float _w, float _h) {
    w = (_w > 0) ? _w : ofGetWidth();
    h = (_h > 0) ? _h : ofGetHeight();

    for (int i = 0; i < (int)drawCmds.size(); i++) {
        drawCmds[i].setSize(w, h);
    }
}

void TelidonDraw::setProgressiveDraw(bool _progressiveDraw) {
    progressiveDraw = _progressiveDraw;
    for (int i = 0; i < (int)drawCmds.size(); i++) {
        drawCmds[i].progressiveDraw = progressiveDraw;
        drawCmds[i].reset();
    }
    finished = drawCmds.empty();
}

void TelidonDraw::setProgressiveDrawInterval(uint64_t _interval) {
    progressiveDrawInterval = _interval;
    for (int i = 0; i < (int)drawCmds.size(); i++) {
        drawCmds[i].progressiveDrawInterval = progressiveDrawInterval;
    }
}

void TelidonDraw::setThickness(float _thickness) {
    thickness = _thickness;
    for (int i = 0; i < (int)drawCmds.size(); i++) {
        drawCmds[i].thickness = thickness;
    }
}

void TelidonDraw::setLabelPoints(bool _labelPoints) {
    labelPoints = _labelPoints;
    for (int i = 0; i < (int)drawCmds.size(); i++) {
        drawCmds[i].labelPoints = labelPoints;
    }
}
