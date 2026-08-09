#include "ofxNaplps/Naplps.h"

#include <algorithm>
#include <cmath>
#include <sstream>

// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
// 0. BINARY UTILITIES
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

std::string nap::binary(int num, int numBits) {
    unsigned int n = (unsigned int)num;
    int bit;

    if (numBits > 0) {
        bit = numBits;
    } else {
        // autodetect, skipping zeros
        bit = 32;
        while (bit > 1 && !((n >> (bit - 1)) & 1)) {
            bit--;
        }
    }

    std::string returns = "";
    while (bit > 0) {
        bit--;
        // anything above bit 31 is zero-padding
        returns += (bit < 32 && ((n >> bit) & 1)) ? '1' : '0';
    }
    return returns;
}

int nap::unbinary(const std::string & binaryString) {
    int i = (int)binaryString.length() - 1;
    unsigned int mask = 1;
    int returns = 0;

    while (i >= 0) {
        char ch = binaryString[i--];
        if (ch != '0' && ch != '1') {
            ofLogError("ofxNaplps") << "unbinary() was passed something that isn't a binary number: " << binaryString;
            return returns;
        }
        if (ch == '1') {
            returns += mask;
        }
        mask <<= 1;
    }
    return returns;
}

std::string nap::decimalToHex(int d, int padding) {
    if (padding <= 0) padding = 8;

    // negative values wrap the way they do in a 32-bit int
    unsigned int u = (unsigned int)d;

    std::stringstream ss;
    ss << std::uppercase << std::hex << u;
    std::string returns = ss.str();

    while ((int)returns.length() < padding) {
        returns = "0" + returns;
    }
    if ((int)returns.length() >= padding) {
        returns = returns.substr(returns.length() - padding, padding);
    }
    return returns;
}

std::string nap::hex(int value, int len) {
    return nap::decimalToHex(value, len);
}

int nap::unhex(const std::string & hexString) {
    unsigned int value = 0;
    std::stringstream ss;
    ss << std::hex << hexString;
    ss >> value;
    return (int)value; // correct for int overflow java expectation
}

float nap::remap(float value, float min1, float max1, float min2, float max2) {
    float range1 = max1 - min1;
    float range2 = max2 - min2;
    if (range1 == 0.0f) return min2;
    float valueScaled = (value - min1) / range1;
    return min2 + (valueScaled * range2);
}

float nap::getDistance(const glm::vec2 & v1, const glm::vec2 & v2) {
    return glm::distance(v1, v2);
}

float nap::getDistance(const ofColor & c1, const ofColor & c2) {
    float dr = (float)c1.r - (float)c2.r;
    float dg = (float)c1.g - (float)c2.g;
    float db = (float)c1.b - (float)c2.b;
    return sqrtf((dr * dr) + (dg * dg) + (db * db));
}

std::string nap::removeCharAt(const std::string & s, int index) {
    std::string returns = "";
    for (int i = 0; i < (int)s.length(); i++) {
        if (i != index) returns += s[i];
    }
    return returns;
}

char nap::doEncode(const std::string & input) {
    if (input.length() < 2) return (char)0;
    std::string pair = input.substr(input.length() - 2, 2);
    return (char)nap::unhex(pair);
}

const std::vector<ofColor> & nap::defaultColorMap() {
    static const std::vector<ofColor> colorMap = {
        ofColor(0, 0, 0),          // black
        ofColor(32, 32, 32),       // gray1
        ofColor(64, 64, 64),       // gray2
        ofColor(96, 96, 96),       // gray3
        ofColor(128, 128, 128),    // gray4
        ofColor(160, 160, 160),    // gray5
        ofColor(192, 192, 192),    // gray6
        ofColor(224, 224, 224),    // gray7
        ofColor(0, 0, 255),        // blue, index 60
        ofColor(5*36, 0, 7*36),    // blue magenta
        ofColor(7*36, 0, 4*36),    // pinkish red
        ofColor(7*36, 2*36, 0),    // orange red
        ofColor(255, 255, 0),      // yellow
        ofColor(2*36, 7*36, 0),    // yellow green
        ofColor(0, 7*36, 4*36),    // greenish
        ofColor(0, 5*36, 7*36)     // bluegreen
    };
    return colorMap;
}

const std::vector<std::string> & nap::defaultColorIndices1() {
    static const std::vector<std::string> indices = {
        "40", "44", "49", "4D", "52", "56", "5B", "5F",
        "60", "64", "68", "6C", "70", "74", "78", "7C"
    };
    return indices;
}

const std::vector<std::string> & nap::defaultColorIndices2() {
    static const std::vector<std::string> indices = {
        "40", "60", "40", "60", "50", "70", "50", "70",
        "40", "40", "40", "40", "40", "40", "40", "40"
    };
    return indices;
}

// white isn't part of the default palette, but it's the reset color
static const ofColor NAP_WHITE = ofColor(255, 255, 255);
static const ofColor NAP_YELLOW = ofColor(255, 255, 0);
static const ofColor NAP_BLACK = ofColor(0, 0, 0);

// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
// 1. SINGLE-BYTE data classes
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

NapChar::NapChar() {
    c = 0;
    decodeChar();
}

NapChar::NapChar(char _c) {
    c = _c;
    decodeChar();
}

void NapChar::decodeChar() {
    ascii = (int)((unsigned char)c);

    // the low 7 bits, the way the JS version slices them out of a 16-bit string
    std::string b = nap::binary(ascii, 16);
    binary = b.substr(b.length() - 7, 7);

    rbinary = binary;
    std::reverse(rbinary.begin(), rbinary.end());

    std::string h = nap::hex(ascii, 8);
    hex = h.substr(h.length() - 2, 2);
}

// ~ ~ ~ 1.2. Opcodes ~ ~ ~

// *** IMPORTANT STEP 1 of 3 ***
// This is the first step, where we match the hex code to a command.
// The second step happens in the NapCmd constructor.
struct NapOpcodeEntry {
    const char * hex;
    NapOpcodeId opId;
    const char * name;
};

static const NapOpcodeEntry napOpcodeTable[] = {
    //~ ~ ~ ~ ~ CONTROL CODES ~ ~ ~ ~ ~
    { "0E", NAP_OP_SHIFT_OUT, "Shift-Out" }, // graphics mode, we're here by default. Handled in step 2.
    { "0F", NAP_OP_SHIFT_IN,  "Shift-In" },  // text mode, data that follows is text. Handled in step 2.
    { "18", NAP_OP_CANCEL,    "CANCEL" },
    { "1B", NAP_OP_ESC,       "ESC" },
    { "1F", NAP_OP_NSR,       "NSR" },       // Non-Selective Reset
    //~ ~ ~ ~ ~ PDI (PICTURE DESCRIPTION INSTRUCTION) CODES ~ ~ ~ ~ ~
    //~ ~ ~ ENVIRONMENT, part 1 ~ ~ ~
    { "20", NAP_OP_RESET,     "RESET" },
    { "21", NAP_OP_DOMAIN,    "DOMAIN" },    // header information
    { "22", NAP_OP_TEXT,      "TEXT" },      // formats text, doesn't contain text itself
    { "23", NAP_OP_TEXTURE,   "TEXTURE" },
    //~ ~ ~ POINTS ~ ~ ~
    { "24", NAP_OP_POINT_SET_ABS, "POINT SET ABS" },
    { "25", NAP_OP_POINT_SET_REL, "POINT SET REL" },
    { "26", NAP_OP_POINT_ABS,     "POINT ABS" },
    { "27", NAP_OP_POINT_REL,     "POINT REL" },
    //~ ~ ~ LINES ~ ~ ~
    { "28", NAP_OP_LINE_ABS,      "LINE ABS" },
    { "29", NAP_OP_LINE_REL,      "LINE REL" },
    { "2A", NAP_OP_SET_LINE_ABS,  "SET & LINE ABS" },
    { "2B", NAP_OP_SET_LINE_REL,  "SET & LINE REL" },
    //~ ~ ~ ARCS ~ ~ ~
    { "2C", NAP_OP_ARC_OUTLINED,     "ARC OUTLINED" },
    { "2D", NAP_OP_ARC_FILLED,       "ARC FILLED" },
    { "2E", NAP_OP_SET_ARC_OUTLINED, "SET & ARC OUTLINED" },
    { "2F", NAP_OP_SET_ARC_FILLED,   "SET & ARC FILLED" },
    //~ ~ ~ RECTANGLES ~ ~ ~
    { "30", NAP_OP_RECT_OUTLINED,     "RECT OUTLINED" },
    { "31", NAP_OP_RECT_FILLED,       "RECT FILLED" },
    { "32", NAP_OP_SET_RECT_OUTLINED, "SET & RECT OUTLINED" },
    { "33", NAP_OP_SET_RECT_FILLED,   "SET & RECT FILLED" },
    //~ ~ ~ POLYGONS ~ ~ ~
    { "34", NAP_OP_POLY_OUTLINED,     "POLY OUTLINED" },
    { "35", NAP_OP_POLY_FILLED,       "POLY FILLED" },
    { "36", NAP_OP_SET_POLY_OUTLINED, "SET & POLY OUTLINED" },
    { "37", NAP_OP_SET_POLY_FILLED,   "SET & POLY FILLED" },
    //~ ~ ~ INCREMENTALS ~ ~ ~
    { "38", NAP_OP_FIELD,                    "FIELD" },
    { "39", NAP_OP_INCREMENTAL_POINT,        "INCREMENTAL POINT" },
    { "3A", NAP_OP_INCREMENTAL_LINE,         "INCREMENTAL LINE" },
    { "3B", NAP_OP_INCREMENTAL_POLY_FILLED,  "INCREMENTAL POLY FILLED" },
    //~ ~ ~ ENVIRONMENT, part 2 ~ ~ ~
    { "3C", NAP_OP_SET_COLOR,    "SET COLOR" },
    { "3D", NAP_OP_WAIT,         "WAIT" },
    { "3E", NAP_OP_SELECT_COLOR, "SELECT COLOR" },
    { "3F", NAP_OP_BLINK,        "BLINK" }
};

static const int napOpcodeTableSize = (int)(sizeof(napOpcodeTable) / sizeof(NapOpcodeEntry));

NapOpcodeId NapOpcode::getOpId(const std::string & hex) {
    for (int i = 0; i < napOpcodeTableSize; i++) {
        if (hex == napOpcodeTable[i].hex) return napOpcodeTable[i].opId;
    }
    return NAP_OP_NONE;
}

std::string NapOpcode::getIdName(NapOpcodeId opId) {
    if (opId == NAP_OP_NONE) return "";
    for (int i = 0; i < napOpcodeTableSize; i++) {
        if (opId == napOpcodeTable[i].opId) return napOpcodeTable[i].name;
    }
    return "";
}

NapOpcode::NapOpcode() : NapChar() {
    opId = getOpId(hex);
    id = getIdName(opId);
}

NapOpcode::NapOpcode(char _c) : NapChar(_c) {
    opId = getOpId(hex);
    id = getIdName(opId);
}

// ~ ~ ~ 1.3. Data ~ ~ ~

NapData::NapData() : NapChar() { }

NapData::NapData(char _c) : NapChar(_c) { }

float NapData::getNormFloat() const {
    return (float)ascii / 127.0f;
}

// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
// 2. MULTI-BYTE data classes
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

NapDataArray::NapDataArray(const std::vector<NapData> & n) {
    bitsPerByte = 3;
    firstBitSign = true;
    bitVals = getBitValsSigned(n);
}

double NapDataArray::getBitValsUnsigned(const std::vector<NapData> & n) const {
    return pow(2.0, (double)((int)n.size() * bitsPerByte));
}

double NapDataArray::getBitValsSigned(const std::vector<NapData> & n) const {
    return pow(2.0, (double)(((int)n.size() * bitsPerByte) - (firstBitSign ? 1 : 0)));
}

float NapDataArray::getSign(char c) {
    if (c == '1') {
        return -1.0f;
    } else {
        return 1.0f;
    }
}

std::string NapDataArray::binaryConv(const NapData & n, int loc) const {
    std::string returns = "";
    for (int i = loc; i < loc + bitsPerByte; i++) {
        if (i >= 0 && i < (int)n.binary.length()) returns += n.binary[i];
    }
    return returns;
}

// ~ ~ ~ 2.2. Vectors ~ ~ ~

NapVector::NapVector(const std::vector<NapData> & n) : NapDataArray(n) {
    x = getCoordFromBytes(n, "x");
    y = getCoordFromBytes(n, "y");
    //z = getCoordFromBytes(n, "z");
}

std::string NapVector::getSingleByteVal(const NapData & n, const std::string & axis) const {
    if (axis == "x") {
        return binaryConv(n, 1);
    } else if (axis == "y") {
        return binaryConv(n, bitsPerByte + 1);
    } else if (axis == "z") {
        return binaryConv(n, (2 * bitsPerByte) + 1); // ? untested
    }
    return "";
}

float NapVector::getCoordFromBytes(const std::vector<NapData> & n, const std::string & axis) const {
    std::string returns = "";
    for (int i = 0; i < (int)n.size(); i++) {
        returns += getSingleByteVal(n[i], axis);
    }

    float sign = 1.0f;
    if (firstBitSign && returns.length() > 0) {
        sign = getSign(returns[0]);
        returns = nap::removeCharAt(returns, 0);
    }

    float finalReturns = 0.0f;

    if (axis == "x") {
        finalReturns = ((float)nap::unbinary(returns) / (float)bitVals) * sign;
    } else if (axis == "y") {
        finalReturns = (((float)bitVals - (float)nap::unbinary(returns)) / (float)bitVals) * sign;
    } else if (axis == "z") {
        finalReturns = ((float)nap::unbinary(returns) / (float)bitVals) * sign; // ? untested
    }

    return finalReturns;
}

// ~ ~ ~ 2.3. Text ~ ~ ~

NapText::NapText(const std::vector<NapData> & n) : NapDataArray(n) {
    text = setTextFromBytes(n);
}

std::string NapText::setTextFromBytes(const std::vector<NapData> & n) const {
    std::string returns = "";
    for (int i = 0; i < (int)n.size(); i++) {
        returns += n[i].c;
    }
    return returns;
}

// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
// 3. STATE
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

NapState::NapState() {
    verbose = false;
    reset();
}

void NapState::reset() {
    drawingCursor = glm::vec2(0.0f, 0.0f);
    colorMap = nap::defaultColorMap();
    colorMode = 0;
    lastColor = NAP_WHITE;
    lastIndex = 0;

    backgroundColor = NAP_BLACK;
    drawBackground = true;
    singleValLength = 1;
    multiValLength = 3;
    minVal = 40; // 64
    is3D = false;
}

// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
// 4. COMMAND
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

NapCmd::NapCmd() {
    pointBytes = 3;
    singleBytes = 1;
    index = 0;
    col = NAP_WHITE;
    text = "";
}

NapCmd::NapCmd(const std::string & _cmdRaw, int _index, NapState & state) {
    pointBytes = state.multiValLength;
    singleBytes = state.singleValLength;
    cmdRaw = _cmdRaw;
    index = _index;
    col = state.lastColor;
    text = "";

    opcode = NapOpcode(cmdRaw.length() > 0 ? cmdRaw[0] : (char)0);
    for (int i = 1; i < (int)cmdRaw.length(); i++) {
        data.push_back(NapData(cmdRaw[i]));
    }

    // *** IMPORTANT STEP 2 of 3 ***
    // The second step is where we find out what kind of command it is,
    // which tells us how we handle the data.
    // The third and final step is done separately, in the drawing code.
    switch (opcode.opId) {
        //~ ~ ~ ~ ~ CONTROL CODES ~ ~ ~ ~ ~
        case NAP_OP_SHIFT_OUT: // graphics mode, we're here by default
            // no effect?
            break;
        case NAP_OP_SHIFT_IN: // text mode, data that follows is text
            setText();
            break;
        case NAP_OP_CANCEL:
            // no effect?
            break;
        case NAP_OP_ESC:
            // no effect?
            break;
        case NAP_OP_NSR: // Non-Selective Reset
            sendNsr(state);
            break;
        //~ ~ ~ ~ ~ PDI (PICTURE DESCRIPTION INSTRUCTION) CODES ~ ~ ~ ~ ~
        //~ ~ ~ ENVIRONMENT, part 1 ~ ~ ~
        case NAP_OP_RESET:
            sendReset(state);
            break;
        case NAP_OP_DOMAIN: // header information
            setDomain(state);
            break;
        case NAP_OP_TEXT: // formats text, doesn't contain text itself
            // TODO
            break;
        case NAP_OP_TEXTURE:
            // TODO
            break;
        //~ ~ ~ POINTS ~ ~ ~
        case NAP_OP_POINT_SET_ABS:
            setPoints(false, true, state); // relative, set cursor
            break;
        case NAP_OP_POINT_SET_REL:
            setPoints(true, true, state);
            break;
        case NAP_OP_POINT_ABS:
            setPoints(false, false, state);
            break;
        case NAP_OP_POINT_REL:
            setPoints(true, false, state);
            break;
        //~ ~ ~ LINES ~ ~ ~
        case NAP_OP_LINE_ABS:
            setPoints(true, false, state); // TODO why is this broken?
            break;
        case NAP_OP_LINE_REL:
            setPoints(true, false, state);
            break;
        case NAP_OP_SET_LINE_ABS:
            setPoints(true, true, state); // TODO why is this broken?
            break;
        case NAP_OP_SET_LINE_REL:
            setPoints(true, true, state);
            break;
        //~ ~ ~ ARCS ~ ~ ~
        case NAP_OP_ARC_OUTLINED:
            setPoints(true, false, state);
            break;
        case NAP_OP_ARC_FILLED:
            setPoints(true, false, state);
            break;
        case NAP_OP_SET_ARC_OUTLINED:
            setPoints(false, true, state);
            break;
        case NAP_OP_SET_ARC_FILLED:
            setPoints(false, true, state);
            break;
        //~ ~ ~ RECTANGLES ~ ~ ~
        case NAP_OP_RECT_OUTLINED:
            setPoints(true, false, state);
            break;
        case NAP_OP_RECT_FILLED:
            setPoints(true, false, state);
            break;
        case NAP_OP_SET_RECT_OUTLINED:
            setPoints(false, true, state);
            break;
        case NAP_OP_SET_RECT_FILLED:
            setPoints(false, true, state);
            break;
        //~ ~ ~ POLYGONS ~ ~ ~
        case NAP_OP_POLY_OUTLINED:
            setPoints(true, false, state);
            break;
        case NAP_OP_POLY_FILLED:
            setPoints(true, false, state);
            break;
        case NAP_OP_SET_POLY_OUTLINED: // relative points after first
            setPoints(false, true, state);
            break;
        case NAP_OP_SET_POLY_FILLED: // relative points after first
            setPoints(false, true, state);
            break;
        //~ ~ ~ INCREMENTALS ~ ~ ~
        case NAP_OP_FIELD:
            // TODO
            break;
        case NAP_OP_INCREMENTAL_POINT:
            setPoints(true, true, state);
            break;
        case NAP_OP_INCREMENTAL_LINE:
            setPoints(true, true, state);
            break;
        case NAP_OP_INCREMENTAL_POLY_FILLED:
            setPoints(true, true, state);
            break;
        //~ ~ ~ ENVIRONMENT, part 2 ~ ~ ~
        case NAP_OP_SET_COLOR:
            setColor(state);
            break;
        case NAP_OP_WAIT:
            // TODO
            break;
        case NAP_OP_SELECT_COLOR:
            selectColor(state); // palette color
            break;
        case NAP_OP_BLINK:
            // TODO
            break;
        default:
            break;
    }
}

void NapCmd::printCmd(const std::string & mode) const {
    ofLogNotice("ofxNaplps") << formatCmd(mode);
}

std::string NapCmd::formatCmd(const std::string & mode) const {
    std::string returns = "(" + ofToString(index + 1) + ") " + opcode.id;
    if (data.size() > 0) returns += ": ";

    if (opcode.id == "") {
        if (mode == "char") {
            returns += opcode.c;
        } else if (mode == "binary") {
            returns += opcode.binary;
        } else if (mode == "rbinary") {
            returns += opcode.rbinary;
        } else if (mode == "ascii") {
            returns += ofToString(opcode.ascii);
        } else if (mode == "hex") {
            returns += opcode.hex;
        }
    }

    if (data.size() > 0) {
        if (opcode.id == "") returns += ", ";
        for (int i = 0; i < (int)data.size(); i++) {
            if (mode == "char") {
                returns += data[i].c;
            } else if (mode == "binary") {
                returns += data[i].binary;
            } else if (mode == "rbinary") {
                returns += data[i].rbinary;
            } else if (mode == "ascii") {
                returns += ofToString(data[i].ascii);
            } else if (mode == "hex") {
                returns += data[i].hex;
            }
            if (i < (int)data.size() - 1) returns += ", ";
        }
    }
    return returns;
}

// ~ ~ ~ Parsing methods begin here ~ ~ ~

bool NapCmd::setColor(NapState & state) {
    int r = 0, g = 0, b = 0;
    int r2 = 0, g2 = 0, b2 = 0;
    int colorValLength = (int)data.size();
    int shift = 8 - (2 * colorValLength);
    state.lastColor = NAP_YELLOW; // default

    if (colorValLength < 1) {
        col = state.lastColor;
        return false;
    }

    int c = data[0].ascii;
    if (c < state.minVal) {
        col = state.lastColor;
        return false;
    }

    g = c & 040;
    r = c & 020;
    b = c & 010;
    c <<= 2;
    g |= c & 020;
    r |= c & 010;
    b |= c & 004;
    g >>= 4;
    r >>= 3;
    b >>= 2;

    for (int i = 1; i < colorValLength; i++) {
        c = data[i].ascii;
        if (c < state.minVal) {
            col = state.lastColor;
            return false;
        }
        g2 = c & 040;
        r2 = c & 020;
        b2 = c & 010;
        c <<= 2;
        g2 |= c & 020;
        r2 |= c & 010;
        b2 |= c & 004;
        g2 >>= 4;
        r2 >>= 3;
        b2 >>= 2;
        g <<= 2;
        r <<= 2;
        b <<= 2;
        g |= g2;
        r |= r2;
        b |= b2;
    }

    // JS lets a >4-byte color operand shift by a negative amount, which is
    // undefined behavior in C++. Clamping keeps the low bits instead.
    if (shift < 0) shift = 0;

    int fill = 0; //(2 << shift) - 1;
    r <<= shift;
    g <<= shift;
    b <<= shift;

    state.lastColor = ofColor(
        (unsigned char)ofClamp(r + fill, 0, 255),
        (unsigned char)ofClamp(g + fill, 0, 255),
        (unsigned char)ofClamp(b + fill, 0, 255)
    );

    if (state.verbose) {
        std::string action = (state.colorMode != 0) ? "<palette write>" : "<palette read>";
        ofLogNotice("ofxNaplps") << action << " index: " << state.lastIndex
            << ", color: " << (int)state.lastColor.r << " " << (int)state.lastColor.g << " " << (int)state.lastColor.b;
    }

    if (state.colorMode != 0 && state.lastIndex >= 0 && state.lastIndex < (int)state.colorMap.size()) {
        state.colorMap[state.lastIndex] = state.lastColor;
    }

    col = state.lastColor;
    return true;
}

bool NapCmd::selectColor(NapState & state) {
    if (data.size() < 1) {
        state.colorMode = 0;
        col = state.lastColor;
        return false;
    }

    int c = data[0].ascii;
    if (c < state.minVal) {
        state.colorMode = 0;
        return false;
    }

    state.lastIndex = (c & 074) >> 2;
    state.colorMode = 1;
    if (state.lastIndex >= 0 && state.lastIndex < (int)state.colorMap.size()) {
        state.lastColor = state.colorMap[state.lastIndex];
    }
    //ignore mode 2 for now

    if (state.verbose) {
        ofLogNotice("ofxNaplps") << "<palette read> index: " << state.lastIndex
            << ", color: " << (int)state.lastColor.r << " " << (int)state.lastColor.g << " " << (int)state.lastColor.b;
    }

    col = state.lastColor;
    return true;
}

void NapCmd::setPoints(bool allPointsRelative, bool setCursor, NapState & state) {
    std::vector<NapVector> nvList;

    // The JS version throws (and loses every point in the command) if the operand
    // bytes don't divide evenly into coordinates. Same outcome here, without the throw.
    bool ok = (pointBytes > 0) && ((int)data.size() % pointBytes == 0);

    if (ok) {
        for (int i = 0; i + pointBytes <= (int)data.size(); i += pointBytes) {
            std::vector<NapData> n(data.begin() + i, data.begin() + i + pointBytes);
            nvList.push_back(NapVector(n));
        }

        for (int i = 0; i < (int)nvList.size(); i++) {
            const NapVector & nv = nvList[i];

            if (!allPointsRelative && i == 0) {
                points.push_back(glm::vec2(nv.x, nv.y));
                if (state.verbose) {
                    ofLogNotice("ofxNaplps") << "Starting with first point... " << (i + 1)
                        << ". Decoded initial point (" << nv.x << ", " << nv.y << ").";
                }
            } else if (allPointsRelative && i == 0) {
                // TODO find something to test this
                points.push_back(state.drawingCursor);
                if (state.verbose) {
                    ofLogNotice("ofxNaplps") << "Starting with cursor position... " << (i + 1)
                        << ". Using cursor initial point (" << state.drawingCursor.x << ", " << state.drawingCursor.y << ").";
                }
            } else {
                glm::vec2 p = points.back();

                // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
                float x = fabs(nv.x) + fabs(p.x);
                if (nv.x < 0) x -= 1.0f;

                float y = fabs(nv.y) + fabs(p.y);
                if (nv.y >= 0) y -= 1.0f;
                // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

                if (state.verbose) {
                    ofLogNotice("ofxNaplps") << (i + 1) << ". Decoded point (" << x << ", " << y << ").";
                }
                points.push_back(glm::vec2(x, y));
            }
        }
    } else {
        ofLogVerbose("ofxNaplps") << "*Error* " << opcode.id << " contains no coordinates.";
    }

    if (setCursor) {
        if (points.size() > 0) {
            state.drawingCursor = points.back();
        } else {
            ofLogVerbose("ofxNaplps") << "*Error* " << opcode.id << " tried to set cursor position but failed.";
        }
    }
}

void NapCmd::sendReset(NapState & state) {
    if (data.size() < 1) return;

    int c = data[0].ascii;
    if (c < state.minVal) {
        return;
    }
    if ((c & 001) != 0) { // reset domain
        state.singleValLength = 1;
        state.multiValLength = 3;
        state.is3D = false;
    }

    int colormodeVal = ((c & 005) >> 1);
    switch (colormodeVal) {
        case 0:
            break;
        case 1:
            state.colorMode = 0;
            break;
        case 2:
            state.colorMode = 1;
            break;
        case 3:
            state.colorMode = 1;
            state.lastColor = NAP_WHITE;
            break;
    }

    int screenVal = ((c & 070) >> 3);
    switch (screenVal) {
        case 0:
            break;
        case 1:
        case 7:
            // clear the screen: left to the app, which owns the background
            break;
        case 2:
        case 5:
        case 6:
            // fill the screen: left to the app, which owns the background.
            // The JS version dies here on an undefined `screenstuff` variable and
            // abandons the rest of the reset, so we return for the same result.
            return;
        case 3:
            // outline the screen in black
            break;
        case 4:
            // outline the screen
            break;
    }

    if (data.size() < 2) return;

    c = data[1].ascii;
    if (c < state.minVal) {
        return;
    }
    if ((c & 001) != 0) { // reset text
        state.drawingCursor = glm::vec2(0.0f, 0.0f);
    }
    if ((c & 010) != 0) { // reset texture
        //state.highlight = false;
        //state.lineTexture = 0;
        //state.texturePattern = 0;
    }
    // for now ignore the rest
}

void NapCmd::sendNsr(NapState & state) {
    state.colorMode = 0;
    state.colorMap = nap::defaultColorMap();
    state.lastColor = NAP_WHITE;
}

void NapCmd::setText() {
    NapText nt(data);
    text += nt.text;
}

void NapCmd::setDomain(NapState & state) {
    /*
    Only the first byte after the domain opcode (21) is used here.
    The rest of the bytes control "logical pel size" (the ability to render images at a
    different resolution than the display, in this case the app window).
    Logical pel size is not implemented.

    In the domain byte, bits 6,5,4,3,2,1 contain the following information:
    * Bit 6 controls 2D vs. 3D coordinates:
            0  XY (the default)
            1  XYZ

    * Bits 5, 4, 3 control the length of a multi-value operand:
            0 0 0   1 byte
            0 0 1   2 bytes
            0 1 0   3 bytes (the default)
            0 1 1   4 bytes
            1 0 0   5 bytes
            1 0 1   6 bytes
            1 1 0   7 bytes
            1 1 1   8 bytes

    * Bits 2, 1 control the length of a single value operand:
            0 0    1 byte (the default)
            0 1    2 bytes
            1 0    3 bytes
            1 1    4 bytes
    */
    if (data.size() < 1) return;

    const std::string & domainByte = data[0].binary; // 7 digits
    if (domainByte.length() < 7) return;

    std::string domainPointBytes = domainByte.substr(2, 3);
    std::string domainSingleBytes = domainByte.substr(5, 2);

    // TODO find out why this fails in some cases
    if (domainPointBytes == "000") {
        state.multiValLength = 1;
    } else if (domainPointBytes == "001") {
        state.multiValLength = 2;
    } else if (domainPointBytes == "010") {
        state.multiValLength = 3;
    } else if (domainPointBytes == "011") {
        state.multiValLength = 4;
    } else if (domainPointBytes == "100") {
        state.multiValLength = 5;
    } else if (domainPointBytes == "101") {
        state.multiValLength = 6;
    } else if (domainPointBytes == "110") {
        state.multiValLength = 7;
    } else if (domainPointBytes == "111") {
        state.multiValLength = 8;
    } else {
        state.multiValLength = 3;
    }

    if (domainSingleBytes == "00") {
        state.singleValLength = 1;
    } else if (domainSingleBytes == "01") {
        state.singleValLength = 2;
    } else if (domainSingleBytes == "10") {
        state.singleValLength = 3;
    } else if (domainSingleBytes == "11") {
        state.singleValLength = 4;
    } else {
        state.singleValLength = 1;
    }

    ofLogVerbose("ofxNaplps") << "DOMAIN SETTINGS: " << state.multiValLength << " bytes per coordinate.";
}

// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
// 5. DECODER
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

NapDecoder::NapDecoder() {
    version = 709;
}

NapDecoder::NapDecoder(const std::string & input) {
    version = 709;
    decode(input);
}

bool NapDecoder::load(const std::string & filePath) {
    ofFile file(filePath, ofFile::ReadOnly, true); // binary
    if (!file.exists()) {
        ofLogError("ofxNaplps") << "Couldn't find " << filePath;
        return false;
    }

    ofBuffer buffer = ofBufferFromFile(filePath, true); // binary
    fileName = ofFilePath::getFileName(filePath);
    decode(buffer.getText());

    return isLoaded();
}

void NapDecoder::clear() {
    cmds.clear();
    napRaw = "";
    version = 709;
    state.reset();
}

void NapDecoder::decode(const std::string & input) {
    clear();

    napRaw = input;
    parseCommands(napRaw);
    version = detectVersion();

    ofLogNotice("ofxNaplps") << "* * * * * * * * * * *";
    ofLogNotice("ofxNaplps") << "Telidon " << version << " file containing " << cmds.size() << " commands.";
    ofLogNotice("ofxNaplps") << "* * * * * * * * * * *";
}

int NapDecoder::detectVersion() const {
    /* 699 (Telidon) was the first version of the format.
     * It should only be encountered in files created on original Telidon hardware.
     * 709 (NAPLPS) was the second version of the format.
     * It was used in all later software applications, including Prodigy.
     */
    if (cmds.size() < 1) return 709;

    if (cmds[0].opcode.hex == "0E") { // TODO find additional cases
        return 699;
    } else {
        return 709;
    }
}

bool NapDecoder::isOpcode(char c) const {
    // an opcode is any byte with bit 7 clear
    return (((int)((unsigned char)c) >> 6) & 1) == 0;
}

void NapDecoder::parseCommands(const std::string & input) {
    int counter = 0;
    std::string tempCmd = "";

    for (int i = 0; i < (int)input.length(); i++) {
        char c = input[i];
        if (isOpcode(c)) {
            if (tempCmd == "") {
                tempCmd += c;
            } else {
                if (tempCmd.length() >= 1) {
                    cmds.push_back(NapCmd(tempCmd, counter, state));
                    counter++;
                }
                tempCmd = "";
                tempCmd += c;
            }
        } else {
            tempCmd += c;
        }
    }

    if (state.verbose) {
        for (int i = 0; i < (int)cmds.size(); i++) {
            cmds[i].printCmd("hex");
        }
    }
}

// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
// 6. ENCODER
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

NapInputWrapper::NapInputWrapper() {
    color = ofColor(255, 255, 255);
    isFill = false;
}

NapInputWrapper::NapInputWrapper(const ofColor & _color, const std::vector<glm::vec2> & _points, bool _isFill) {
    color = _color;
    points = _points;
    isFill = _isFill;
}

NapEncoder::NapEncoder() {
    dataLength = 4;
    bitsPerByte = 3;
    bitExponent = (dataLength * bitsPerByte) - 1;
    maxBitVals = powf(2.0f, (float)bitExponent);
    firstBitSign = true;
    hasLastColor = false;
    debug = false;
}

std::string NapEncoder::encode(std::vector<NapInputWrapper> strokes, int _dataLength) {
    dataLength = (_dataLength > 0) ? _dataLength : 4;
    bitsPerByte = 3;
    bitExponent = (dataLength * bitsPerByte) - 1;
    maxBitVals = powf(2.0f, (float)bitExponent);
    firstBitSign = true;
    hasLastColor = false;

    ofLogNotice("ofxNaplps") << "Encoder input is " << strokes.size() << " strokes.";

    cmds = generateCommands(strokes);

    napRaw = "";
    for (int i = 0; i < (int)cmds.size(); i++) {
        napRaw += cmds[i];
    }
    return napRaw;
}

std::string NapEncoder::encode(std::vector<NapInputWrapper> strokes, int _dataLength, float normX, float normY) {
    normalizeAllStrokes(strokes, normX, normY);
    return encode(strokes, _dataLength);
}

bool NapEncoder::save(const std::string & filePath) {
    if (napRaw.length() < 1) {
        ofLogError("ofxNaplps") << "Nothing to save, call encode() first.";
        return false;
    }
    ofBuffer buffer(napRaw.c_str(), napRaw.length());
    return ofBufferToFile(filePath, buffer, true); // binary
}

std::vector<std::string> NapEncoder::generateCommands(std::vector<NapInputWrapper> & strokes) {
    std::vector<std::string> returns;

    std::string headerString = makeNapHeader();
    if (debug) ofLogNotice("ofxNaplps") << "Generating header: " << headerString;
    returns.push_back(headerString);

    for (int i = 0; i < (int)strokes.size(); i++) {
        if (debug) ofLogNotice("ofxNaplps") << "* * * Begin encoding stroke " << (i + 1) << " / " << strokes.size() << " * * *";
        returns.push_back(makeNapStroke(strokes[i].isFill, strokes[i].color, strokes[i].points));
        if (debug) ofLogNotice("ofxNaplps") << "* * * End encoding stroke " << (i + 1) << " / " << strokes.size() << " * * *";
    }

    std::string footerString = makeNapFooter();
    if (debug) ofLogNotice("ofxNaplps") << "Generating footer: " << footerString;
    returns.push_back(footerString);

    return returns;
}

std::string NapEncoder::makeNapHeader() const {
    std::string returns = "";

    returns += nap::doEncode("18"); // cancel

    returns += nap::doEncode("1B"); // esc
    returns += nap::doEncode("45");

    returns += nap::doEncode("1F"); // nsr
    returns += nap::doEncode("40");
    returns += nap::doEncode("40");

    returns += nap::doEncode("0E"); // shift-out (graphics mode)

    returns += nap::doEncode("20"); // reset
    returns += nap::doEncode("7F");
    returns += nap::doEncode("4F");

    returns += nap::doEncode("21"); // domain
    // We aren't truly programmatically setting the domain here; just assuming
    // the number of bytes is 3 or 4.
    if (dataLength == 3) {
        returns += nap::doEncode("49");
    } else {
        returns += nap::doEncode("4D");
    }
    returns += nap::doEncode("40");
    returns += nap::doEncode("40");
    returns += nap::doEncode("40");
    returns += nap::doEncode("40");

    return returns;
}

std::string NapEncoder::makeNapFooter() const {
    std::string returns = "";

    returns += nap::doEncode("1B"); // esc
    returns += nap::doEncode("45");

    return returns;
}

std::string NapEncoder::makeNapOpcode(bool isFill) const {
    /*
     * 28 LINE ABS
     * 29 LINE REL
     * 2A SET & LINE ABS
     * 2B SET & LINE REL
     * 34 POLY OUTLINED
     * 35 POLY FILLED
     * 36 SET & POLY OUTLINED
     * 37 SET & POLY FILLED
     */
    // only poly line and fill are implemented
    std::string returns = "";

    if (!isFill) {
        returns += nap::doEncode("36"); // OUTLINED
    } else {
        returns += nap::doEncode("37"); // FILLED
    }

    return returns;
}

// TODO set color can generate a custom 16-color palette

std::string NapEncoder::makeNapSelectColor(const ofColor & color) const {
    std::string returns = "";

    returns += nap::doEncode("3E"); // SELECT COLOR

    int index = 0;
    float dist = 999999.0f;
    const std::vector<ofColor> & colorMap = nap::defaultColorMap();
    for (int i = 0; i < (int)colorMap.size(); i++) {
        float newDist = nap::getDistance(color, colorMap[i]);
        if (newDist < dist) {
            index = i;
            dist = newDist;
        }
    }

    returns += nap::doEncode(nap::defaultColorIndices1()[index]);
    returns += nap::doEncode(nap::defaultColorIndices2()[index]);
    returns += nap::doEncode("40");
    returns += nap::doEncode("40");

    return returns;
}

std::string NapEncoder::makeNapVector2(const glm::vec2 & input) const {
    std::string returns = "";

    if (debug) ofLogNotice("ofxNaplps") << "Encoding vector input " << input.x << ", " << input.y << " ...";

    const int intX = (int)(fabs(input.x) * maxBitVals);
    const int intY = (int)(fabs(input.y) * maxBitVals);
    if (debug) ofLogNotice("ofxNaplps") << "Converting vector to int: " << intX << ", " << intY;

    std::string binX = nap::binary(intX, bitExponent);
    std::string binY = nap::binary(intY, bitExponent);
    if (debug) ofLogNotice("ofxNaplps") << "Converting int to binary: " << binX << ", " << binY;

    for (int i = 0; i < dataLength; i++) {
        std::string vectorByte = "01";

        // The JS version unrolls this into four hardcoded cases, which caps the domain
        // at 4 bytes. The first byte carries the sign plus 2 bits per axis, and every
        // byte after it carries 3 bits per axis, so the pattern generalizes.
        int bit = (i == 0) ? 0 : (3 * i) - 1;
        int numBits = (i == 0) ? 2 : 3;

        if (i == 0) {
            // first bit is the sign
            vectorByte += (input.x > 0) ? "0" : "1";
        }
        for (int j = 0; j < numBits; j++) {
            if (bit + j < (int)binX.length()) vectorByte += binX[bit + j];
        }

        if (i == 0) {
            vectorByte += (input.y > 0) ? "0" : "1";
        }
        for (int j = 0; j < numBits; j++) {
            if (bit + j < (int)binY.length()) vectorByte += binY[bit + j];
        }

        const std::string hexByte = nap::hex(nap::unbinary(vectorByte), 8);
        const char encodedByte = nap::doEncode(hexByte);

        if (debug) ofLogNotice("ofxNaplps") << "Encoded byte " << i << ", binary: " << vectorByte << ", hex: " << hexByte;
        returns += encodedByte;
    }

    return returns;
}

void NapEncoder::normalizeAllStrokes(std::vector<NapInputWrapper> & input, float normX, float normY) const {
    float minX = 0.0f;
    float maxX = 0.0f;
    float minY = 0.0f;
    float maxY = 0.0f;
    float minVal = 0.0f;
    float maxVal = 0.0f;

    for (int i = 0; i < (int)input.size(); i++) {
        for (int j = 0; j < (int)input[i].points.size(); j++) {
            const glm::vec2 & point = input[i].points[j];
            if (point.x < minX) {
                minX = point.x;
            } else if (point.x > maxX) {
                maxX = point.x;
            }
            if (point.y < minY) {
                minY = point.y;
            } else if (point.y > maxY) {
                maxY = point.y;
            }
        }
    }

    if (fabs(maxX - minX) > fabs(maxY - minY)) {
        maxVal = maxX;
        minVal = minX;
    } else {
        maxVal = maxY;
        minVal = minY;
    }

    for (int i = 0; i < (int)input.size(); i++) {
        for (int j = 0; j < (int)input[i].points.size(); j++) {
            glm::vec2 & point = input[i].points[j];
            point.x = nap::remap(point.x, minVal, maxVal, 0.0f, normX); // 1.0
            point.y = nap::remap(point.y, minVal, maxVal, 0.0f, normY); // 0.75
        }
    }
}

std::string NapEncoder::makeNapPoints(std::vector<glm::vec2> & points) const {
    if (debug) ofLogNotice("ofxNaplps") << "Encoding " << points.size() << " points to poly ...";
    std::string returns = "";

    std::vector<glm::vec2> pointsToEncode;

    for (int i = 0; i < (int)points.size(); i++) {
        if (debug) ofLogNotice("ofxNaplps") << points[i].x << ", " << points[i].y;
        points[i].y = 1.0f - points[i].y;

        if (points[i].x >= 0 && points[i].x <= 1 && points[i].y >= 0 && points[i].y <= 1) {
            if (i == 0) {
                pointsToEncode.push_back(points[0]);
            } else {
                const glm::vec2 & nv = points[i];
                const glm::vec2 & nvLast = points[i - 1];

                // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
                float x = fabs(nv.x) - fabs(nvLast.x);
                if (nv.x < nvLast.x) x = fabs(x) - 1.0f;

                float y = fabs(nv.y) - fabs(nvLast.y);
                if (nv.y < nvLast.y) y = fabs(y) - 1.0f;
                // ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

                pointsToEncode.push_back(glm::vec2(x, y));
            }
        }
    }

    for (int i = 0; i < (int)pointsToEncode.size(); i++) {
        returns += makeNapVector2(pointsToEncode[i]);
        if (debug) ofLogNotice("ofxNaplps") << "Encoded point (" << pointsToEncode[i].x << ", " << pointsToEncode[i].y << ").";
    }

    return returns;
}

std::string NapEncoder::makeNapStroke(bool isFill, const ofColor & color, std::vector<glm::vec2> & points) {
    std::string returns = "";

    if (!hasLastColor || nap::getDistance(lastColor, color) > 0.1f) {
        returns += makeNapSelectColor(color);
        lastColor = color;
        hasLastColor = true;
    }

    returns += makeNapOpcode(isFill);
    returns += makeNapPoints(points);

    return returns;
}
