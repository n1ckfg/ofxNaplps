#pragma once

/*
+ + +          NAPLPS for openFrameworks       + + +
+ + +     (part of the TelidonP5 Project)      + + +
+ + +   Nick Fox-Gieg  https://fox-gieg.com    + + +

A port of js/telidon/naplps.js. Contains no rendering code:
the decoder turns a .nap byte stream into NapCmds, and Telidon.h
turns those NapCmds into pixels.

Differences from the JavaScript original:
* The mutable module-level globals (naplps_drawingCursor, naplps_colorMap, ...)
  live in NapState, which is owned by the NapDecoder that is parsing. Decoding is
  sequential, so this behaves like the JS version but lets two files be decoded
  without stepping on each other.
* Colors are ofColor rather than a 0-255 Vector3, points are glm::vec2.
* JS exceptions used as control flow are replaced with explicit bounds checks
  that reproduce the same outcomes.
*/

#include "ofMain.h"

#include <map>
#include <string>
#include <vector>

// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
// 0. BINARY UTILITIES
// Ported from the Processing.js-derived helpers at the top of naplps.js.
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
namespace nap {

    // Converts an int to a string of binary digits. numBits <= 0 autodetects,
    // skipping leading zeros, exactly like Processing's binary().
    std::string binary(int num, int numBits = 0);

    // Converts a string of binary digits back to an int.
    int unbinary(const std::string & binaryString);

    std::string decimalToHex(int d, int padding = 8);

    // Converts an int to a string of hex digits, uppercase, len digits long.
    std::string hex(int value, int len = 8);

    int unhex(const std::string & hexString);

    float remap(float value, float min1, float max1, float min2, float max2);

    float getDistance(const glm::vec2 & v1, const glm::vec2 & v2);
    float getDistance(const ofColor & c1, const ofColor & c2);

    std::string removeCharAt(const std::string & s, int index);

    // Takes the last two characters of a hex string and returns that byte.
    char doEncode(const std::string & input);

    // The 16 colors of the default NAPLPS palette.
    const std::vector<ofColor> & defaultColorMap();

    // The two bytes that select each default palette entry, used by the encoder.
    const std::vector<std::string> & defaultColorIndices1();
    const std::vector<std::string> & defaultColorIndices2();

}

// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
// 1. SINGLE-BYTE data classes
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

// 1.1. The NapChar class is the smallest component of a NAPLPS file.
// It just contains a few methods for decoding one byte.
class NapChar {

    public:

        NapChar();
        NapChar(char _c);
        virtual ~NapChar() {}

        char c;              // the raw byte
        int ascii;           // its value
        std::string binary;  // 7 binary digits
        std::string rbinary; // the same digits, reversed
        std::string hex;     // 2 hex digits

    protected:

        void decodeChar();

};

// The opcodes of the NAPLPS Picture Description Instruction set.
// NapOpcode.id keeps the JS string names for printing; opId is the same
// information in a form that's cheap to switch on.
enum NapOpcodeId {
    NAP_OP_NONE = 0,
    //~ ~ ~ ~ ~ CONTROL CODES ~ ~ ~ ~ ~
    NAP_OP_SHIFT_OUT,
    NAP_OP_SHIFT_IN,
    NAP_OP_CANCEL,
    NAP_OP_ESC,
    NAP_OP_NSR,
    //~ ~ ~ ENVIRONMENT, part 1 ~ ~ ~
    NAP_OP_RESET,
    NAP_OP_DOMAIN,
    NAP_OP_TEXT,
    NAP_OP_TEXTURE,
    //~ ~ ~ POINTS ~ ~ ~
    NAP_OP_POINT_SET_ABS,
    NAP_OP_POINT_SET_REL,
    NAP_OP_POINT_ABS,
    NAP_OP_POINT_REL,
    //~ ~ ~ LINES ~ ~ ~
    NAP_OP_LINE_ABS,
    NAP_OP_LINE_REL,
    NAP_OP_SET_LINE_ABS,
    NAP_OP_SET_LINE_REL,
    //~ ~ ~ ARCS ~ ~ ~
    NAP_OP_ARC_OUTLINED,
    NAP_OP_ARC_FILLED,
    NAP_OP_SET_ARC_OUTLINED,
    NAP_OP_SET_ARC_FILLED,
    //~ ~ ~ RECTANGLES ~ ~ ~
    NAP_OP_RECT_OUTLINED,
    NAP_OP_RECT_FILLED,
    NAP_OP_SET_RECT_OUTLINED,
    NAP_OP_SET_RECT_FILLED,
    //~ ~ ~ POLYGONS ~ ~ ~
    NAP_OP_POLY_OUTLINED,
    NAP_OP_POLY_FILLED,
    NAP_OP_SET_POLY_OUTLINED,
    NAP_OP_SET_POLY_FILLED,
    //~ ~ ~ INCREMENTALS ~ ~ ~
    NAP_OP_FIELD,
    NAP_OP_INCREMENTAL_POINT,
    NAP_OP_INCREMENTAL_LINE,
    NAP_OP_INCREMENTAL_POLY_FILLED,
    //~ ~ ~ ENVIRONMENT, part 2 ~ ~ ~
    NAP_OP_SET_COLOR,
    NAP_OP_WAIT,
    NAP_OP_SELECT_COLOR,
    NAP_OP_BLINK
};

// 1.2. Some NapChars contain opcodes, or drawing commands.
// The NapOpcode class decodes the command.
class NapOpcode : public NapChar {

    public:

        NapOpcode();
        NapOpcode(char _c);

        NapOpcodeId opId;
        std::string id;

        // *** IMPORTANT STEP 1 of 3 ***
        // This is the first step, where we match the hex code to a command.
        // The second step happens later on in this decoder.
        static NapOpcodeId getOpId(const std::string & hex);
        static std::string getIdName(NapOpcodeId opId);

};

// 1.3. The NapChars following an opcode contain the data that
// the command will use. A separate data class is used in case
// we need data-specific methods later.
class NapData : public NapChar {

    public:

        NapData();
        NapData(char _c);

        float getNormFloat() const;

};

// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
// 2. MULTI-BYTE data classes
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

// 2.1. NapDataArray objects need to combine multiple NapData pieces for decoding.
class NapDataArray {

    public:

        NapDataArray(const std::vector<NapData> & n);
        virtual ~NapDataArray() {}

        // defaults for coordinate type.
        // TODO set programatically from domain info based on XY (3 bits) or XYZ (2 bits).
        // However in almost all cases we can assume XY.
        int bitsPerByte;
        bool firstBitSign; // should be true for all domain options?
        double bitVals;

        double getBitValsUnsigned(const std::vector<NapData> & n) const;
        double getBitValsSigned(const std::vector<NapData> & n) const;

        static float getSign(char c);

        std::string binaryConv(const NapData & n, int loc) const;

};

// 2.2. XY and XYZ position are handled by the NapVector class.
/*
           X     Y               X   Y   Z
     8 7|6 5 4|3 2 1|       8 7|6 5|4 3|2 1|
    -----------------      -----------------
    |?|1|S| | |S| | |      |?|1|S| |S| |S| |
    -----------------      -----------------
    |?|1| | | | | | |      |?|1| | | | | | |
    -----------------      -----------------
        . . .                  . . .
    -----------------      -----------------
    |?|1| | | | | | |      |?|1| | | | | | |
    -----------------      -----------------

         G R B G R B
     8 7|6 5 4|3 2 1|
    -----------------
    |?|1| | | | | | |
    -----------------
    |?|1| | | | | | |
    -----------------
        . . .
    -----------------
    |?|1| | | | | | |
    -----------------
*/
class NapVector : public NapDataArray {

    public:

        NapVector(const std::vector<NapData> & n);

        float x;
        float y;
        //float z; // untested, the 3D domain was never used in the wild

    private:

        std::string getSingleByteVal(const NapData & n, const std::string & axis) const;
        float getCoordFromBytes(const std::vector<NapData> & n, const std::string & axis) const;

};

// 2.3. Text
class NapText : public NapDataArray {
    // TODO figure out text formatting

    public:

        NapText(const std::vector<NapData> & n);

        std::string text;

    private:

        std::string setTextFromBytes(const std::vector<NapData> & n) const;

};

// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
// 3. STATE
// The module-level globals of naplps.js, gathered up so that a decoder owns them.
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
class NapState {

    public:

        NapState();

        void reset();

        glm::vec2 drawingCursor;
        std::vector<ofColor> colorMap;
        int colorMode;
        ofColor lastColor;
        int lastIndex;

        ofColor backgroundColor;
        bool drawBackground;
        int singleValLength;
        int multiValLength;
        int minVal;
        bool is3D;

        bool verbose; // the JS version console.logs every command and point

};

// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
// 4. COMMAND: One decoded drawing command.
// Assembled from the opcode and data bytes.
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
class NapCmd {

    public:

        NapCmd();
        NapCmd(const std::string & _cmdRaw, int _index, NapState & state);

        int pointBytes;
        int singleBytes;
        std::string cmdRaw;
        int index;
        std::vector<NapData> data;
        std::vector<glm::vec2> points;
        ofColor col;
        std::string text;
        NapOpcode opcode;

        // This prints out the command contents in various formats.
        // Helpful for debugging. Modes: char, binary, rbinary, ascii, hex.
        void printCmd(const std::string & mode = "hex") const;
        std::string formatCmd(const std::string & mode = "hex") const;

    private:

        // ~ ~ ~ Parsing methods begin here ~ ~ ~
        bool setColor(NapState & state);
        bool selectColor(NapState & state);
        void setPoints(bool allPointsRelative, bool setCursor, NapState & state);
        void sendReset(NapState & state);
        void sendNsr(NapState & state);
        void setText();
        void setDomain(NapState & state);

};

// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
// 5. DECODER: Contains all the decoded drawing commands.
// Decodes the format only; drawing happens in TelidonDraw.
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
class NapDecoder {

    public:

        NapDecoder();
        NapDecoder(const std::string & input);

        // Reads a .nap file from bin/data (or any absolute path).
        bool load(const std::string & filePath);

        // Decodes a .nap byte stream that's already in memory.
        void decode(const std::string & input);

        void clear();

        /* 699 (Telidon) was the first version of the format.
         * It should only be encountered in files created on original Telidon hardware.
         * 709 (NAPLPS) was the second version of the format.
         * It was used in all later software applications, including Prodigy.
         */
        int detectVersion() const;

        bool isOpcode(char c) const;

        void setVerbose(bool _verbose) { state.verbose = _verbose; }
        bool isLoaded() const { return !cmds.empty(); }

        std::string napRaw;
        std::vector<NapCmd> cmds;
        int version;
        std::string fileName;
        NapState state;

    private:

        void parseCommands(const std::string & input);

};

// The decoder is the front door of the library, so it gets the friendly name.
typedef NapDecoder Naplps;

// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
// 6. ENCODER
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
class NapInputWrapper {

    public:

        NapInputWrapper();
        NapInputWrapper(const ofColor & _color, const std::vector<glm::vec2> & _points, bool _isFill);

        ofColor color;
        std::vector<glm::vec2> points;
        bool isFill;

};

class NapEncoder {

    public:

        NapEncoder();

        // The number of bytes per encoded vector is set by the domain,
        // and lets you trade precision for file size. The most common value
        // found in the wild is 4, followed by 3.
        std::string encode(std::vector<NapInputWrapper> strokes, int dataLength = 4);

        // Same, but remap all the points into a normX by normY box first,
        // e.g. (1.0, 0.75) for the 4:3 NAPLPS unit screen.
        std::string encode(std::vector<NapInputWrapper> strokes, int dataLength, float normX, float normY);

        bool save(const std::string & filePath);

        std::string napRaw;
        std::vector<std::string> cmds;
        bool debug;

    private:

        std::vector<std::string> generateCommands(std::vector<NapInputWrapper> & strokes);
        std::string makeNapHeader() const;
        std::string makeNapFooter() const;
        std::string makeNapOpcode(bool isFill) const;
        std::string makeNapSelectColor(const ofColor & color) const;
        std::string makeNapVector2(const glm::vec2 & input) const;
        std::string makeNapPoints(std::vector<glm::vec2> & points) const;
        std::string makeNapStroke(bool isFill, const ofColor & color, std::vector<glm::vec2> & points);
        void normalizeAllStrokes(std::vector<NapInputWrapper> & input, float normX, float normY) const;

        int dataLength;

        // Number of position bits per axis, per byte. All known implementations used
        // 3 bits, so this is hardcoded here, but can still be set in the NAPLPS domain command.
        // The 3D version of the format, not known to have ever been implemented, used 2 bits.
        int bitsPerByte;

        // For example a 4-byte domain has 2048 position values
        // (3 position bits per byte, minus the sign for the first bit)
        int bitExponent;
        float maxBitVals;
        bool firstBitSign;

        ofColor lastColor;
        bool hasLastColor;

};
