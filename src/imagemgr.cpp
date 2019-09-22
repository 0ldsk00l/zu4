/*
 * $Id: imagemgr.cpp 3048 2012-06-23 11:50:23Z twschulz $
 */


#include <vector>

#include "config.h"
#include "debug.h"
#include "error.h"
#include "image.h"
#include "imageloader.h"
#include "imageloader_png.h"
#include "imagemgr.h"
#include "intro.h"
#include "settings.h"
#include "u4file.h"

using std::map;
using std::string;
using std::vector;

Image *screenScale(Image *src, int scale, int n, int filter);

bool ImageInfo::hasBlackBackground()
{
	return this->filetype == "image/x-u4raw";
}


class ImageSet {
public:
    ~ImageSet();

    string name;
    string location;
    string extends;
    map<string, ImageInfo *> info;
};

ImageMgr *ImageMgr::instance = NULL;

ImageMgr *ImageMgr::getInstance() {
    if (instance == NULL) {
        instance = new ImageMgr();
        instance->init();
    }
    return instance;
}

void ImageMgr::destroy() {
    if (instance != NULL) {
        delete instance;
        instance = NULL;
    }
}

ImageMgr::ImageMgr() {
    logger = new Debug("debug/imagemgr.txt", "ImageMgr");
    TRACE(*logger, "creating ImageMgr");

    settings.addObserver(this);
}

ImageMgr::~ImageMgr() {
    settings.deleteObserver(this);

    for (std::map<string, ImageSet *>::iterator i = imageSets.begin(); i != imageSets.end(); i++)
        delete i->second;

    delete logger;
}

void ImageMgr::init() {
    TRACE(*logger, "initializing ImageMgr");

    /*
     * register the "screen" image representing the entire screen
     */
    Image *screen = Image::createScreenImage();
    ImageInfo *screenInfo = new ImageInfo;

    screenInfo->name = "screen";
    screenInfo->filename = "";
    screenInfo->width = screen->width();
    screenInfo->height = screen->height();
    screenInfo->depth = 0;
    screenInfo->prescale = 0;
    screenInfo->filetype = "";
    screenInfo->tiles = 0;
    screenInfo->introOnly = false;
    screenInfo->transparentIndex = -1;
    screenInfo->xu4Graphic = false;
    screenInfo->fixup = FIXUP_NONE;
    screenInfo->image = screen;

    /*
     * register all the images declared in the config files
     */
    const Config *config = Config::getInstance();
    vector<ConfigElement> graphicsConf = config->getElement("graphics").getChildren();
    for (std::vector<ConfigElement>::iterator conf = graphicsConf.begin(); conf != graphicsConf.end(); conf++) {
        if (conf->getName() == "imageset") {
            ImageSet *set = loadImageSetFromConf(*conf);
            imageSets[set->name] = set;

            // all image sets include the "screen" image
            set->info[screenInfo->name] = screenInfo;
        }
    }

    imageSetNames.clear();
    for (std::map<string, ImageSet *>::const_iterator set = imageSets.begin(); set != imageSets.end(); set++)
        imageSetNames.push_back(set->first);

    update(&settings);
}

ImageSet *ImageMgr::loadImageSetFromConf(const ConfigElement &conf) {
    ImageSet *set;

    set = new ImageSet;
    set->name = conf.getString("name");
    set->location = conf.getString("location");
    set->extends = conf.getString("extends");

    TRACE(*logger, string("loading image set ") + set->name);

    vector<ConfigElement> children = conf.getChildren();
    for (std::vector<ConfigElement>::iterator i = children.begin(); i != children.end(); i++) {
        if (i->getName() == "image") {
            ImageInfo *info = loadImageInfoFromConf(*i);
            std::map<string, ImageInfo *>::iterator dup = set->info.find(info->name);
            if (dup != set->info.end()) {
                delete dup->second;
                set->info.erase(dup);
            }
            set->info[info->name] = info;
        }
    }

    return set;
}

ImageInfo *ImageMgr::loadImageInfoFromConf(const ConfigElement &conf) {
    ImageInfo *info;
    static const char *fixupEnumStrings[] = { "none", "intro", "abyss", "abacus", "dungns", "blackTransparencyHack", "fmtownsscreen", NULL };

    info = new ImageInfo;
    info->name = conf.getString("name");
    info->filename = conf.getString("filename");
    info->width = conf.getInt("width", -1);
    info->height = conf.getInt("height", -1);
    info->depth = conf.getInt("depth", -1);
    info->prescale = conf.getInt("prescale");
    info->filetype = conf.getString("filetype");
    info->tiles = conf.getInt("tiles");
    info->introOnly = conf.getBool("introOnly");
    info->transparentIndex = conf.getInt("transparentIndex", -1);

    info->xu4Graphic = conf.getBool("xu4Graphic");
    info->fixup = static_cast<ImageFixup>(conf.getEnum("fixup", fixupEnumStrings));
    info->image = NULL;

    vector<ConfigElement> children = conf.getChildren();
    for (std::vector<ConfigElement>::iterator i = children.begin(); i != children.end(); i++) {
        if (i->getName() == "subimage") {
            SubImage *subimage = loadSubImageFromConf(info, *i);
            info->subImages[subimage->name] = subimage;
        }
    }

    return info;
}

SubImage *ImageMgr::loadSubImageFromConf(const ImageInfo *info, const ConfigElement &conf) {
    SubImage *subimage;
    static int x = 0,
               y = 0,
               last_width = 0,
               last_height = 0;    

    subimage = new SubImage;
    subimage->name = conf.getString("name");    
    subimage->width = conf.getInt("width");
    subimage->height = conf.getInt("height");
    subimage->srcImageName = info->name;
    if (conf.exists("x") && conf.exists("y")) {
        x = subimage->x = conf.getInt("x");
        y = subimage->y = conf.getInt("y");
    }
    else {
        // Automatically increment our position through the base image
        x += last_width;
        if (x >= last_width) {
            x = 0;
            y += last_height;
        }

        subimage->x = x;
        subimage->y = y;
    }

    // "remember" the width and height of this subimage
    last_width = subimage->width;
    last_height = subimage->height;

    return subimage;
}

void ImageMgr::fixupIntro(Image *im, int prescale) {
    const unsigned char *sigData;
    int i, x, y;
    bool alpha = im->isAlphaOn();
    RGBA color;

    sigData = intro->getSigData();
    im->alphaOff();
    if (settings.videoType != "VGA-ALLPNG" && settings.videoType != "new") {
        /* ----------------------------
         * update the position of "and"
         * ---------------------------- */
        im->drawSubRectOn(im, 148 * prescale, 17 * prescale,
                          153 * prescale,
                          17 * prescale,
                          11 * prescale,
                          4 * prescale);
        im->drawSubRectOn(im, 159 * prescale, 17 * prescale,
                          165 * prescale,
                          18 * prescale,
                          1 * prescale,
                          4 * prescale);
        im->drawSubRectOn(im, 160 * prescale, 17 * prescale,
                          164 * prescale,
                          17 * prescale,
                          16 * prescale,
                          4 * prescale);
        /* ---------------------------------------------
         * update the position of "Origin Systems, Inc."
         * --------------------------------------------- */
        im->drawSubRectOn(im, 86 * prescale, 21 * prescale,
                          88 * prescale,
                          21 * prescale,
                          114 * prescale,
                          9 * prescale);
        im->drawSubRectOn(im, 199 * prescale, 21 * prescale,
                          202 * prescale,
                          21 * prescale,
                          6 * prescale,
                          9 * prescale);
        im->drawSubRectOn(im, 207 * prescale, 21 * prescale,
                          208 * prescale,
                          21 * prescale,
                          28 * prescale,
                          9 * prescale);
        /* ---------------------------------------------
         * update the position of "Ultima IV"
         * --------------------------------------------- */
        // move this *prior* to moving "present"
        im->drawSubRectOn(im, 59 * prescale, 33 * prescale,
                          61 * prescale,
                          33 * prescale,
                          204 * prescale,
                          46 * prescale);
        /* ---------------------------------------------
         * update the position of "Quest of the Avatar"
         * --------------------------------------------- */
        im->drawSubRectOn(im, 69 * prescale, 80 * prescale,     // quEst
                          70 * prescale,
                          80 * prescale,
                          11 * prescale,
                          13 * prescale);
        im->drawSubRectOn(im, 82 * prescale, 80 * prescale,     // queST
                          84 * prescale,
                          80 * prescale,
                          27 * prescale,
                          13 * prescale);
        im->drawSubRectOn(im, 131 * prescale, 80 * prescale,    // oF
                          132 * prescale,
                          80 * prescale,
                          11 * prescale,
                          13 * prescale);
        im->drawSubRectOn(im, 150 * prescale, 80 * prescale,    // THE
                          149 * prescale,
                          80 * prescale,
                          40 * prescale,
                          13 * prescale);
        im->drawSubRectOn(im, 166 * prescale, 80 * prescale,    // tHe
                          165 * prescale,
                          80 * prescale,
                          11 * prescale,
                          13 * prescale);
        im->drawSubRectOn(im, 200 * prescale, 80 * prescale,    // AVATAR
                          201 * prescale,
                          80 * prescale,
                          81 * prescale,
                          13 * prescale);
        im->drawSubRectOn(im, 227 * prescale, 80 * prescale,    // avAtar
                          228 * prescale,
                          80 * prescale,
                          11 * prescale,
                          13 * prescale);
    }
    /* -----------------------------------------------------------------------------
     * copy "present" to new location between "Origin Systems, Inc." and "Ultima IV"
     * ----------------------------------------------------------------------------- */
    // do this *after* moving "Ultima IV"
    im->drawSubRectOn(im, 132 * prescale, 33 * prescale,
                      135 * prescale,
                      0 * prescale,
                      56 * prescale,
                      5 * prescale);

    if (alpha)
    {
        im->alphaOn();
    }

    /* ----------------------------
     * erase the original "present"
     * ---------------------------- */
    im->fillRect(135 * prescale, 0 * prescale, 56 * prescale, 5 * prescale, 0, 0, 0);

    /* -------------------------
     * update the colors for VGA
     * ------------------------- */
    if (settings.videoType == "VGA")
    {
        ImageInfo *borderInfo = imageMgr->get(BKGD_BORDERS, true);
//        ImageInfo *charsetInfo = imageMgr->get(BKGD_CHARSET);
        if (!borderInfo)
            xu4_error(XU4_LOG_ERR, "ERROR 1001: Unable to load the \"%s\" data file.\t\n\nIs %s installed?\n\nVisit the XU4 website for additional information.\n\thttp://xu4.sourceforge.net/", BKGD_BORDERS, settings.game.c_str());

        delete borderInfo->image;
        borderInfo->image = NULL;
        borderInfo = imageMgr->get(BKGD_BORDERS, true);

        im->setPaletteFromImage(borderInfo->image);

        // update the color of "and" and "present"
        im->setPaletteIndex(15, im->setColor(226, 226, 255));

        // update the color of "Origin Systems, Inc."
        im->setPaletteIndex(9, im->setColor(129, 129, 255));

        //borderInfo->image->save("border.png");
        // update the border appearance
        borderInfo->image->alphaOff();
        borderInfo->image->drawSubRectOn(im, 0, 96, 0, 0, 16, 56);
        for (int i=0; i < 9; i++)
        {
            borderInfo->image->drawSubRectOn(im, 16+(i*32), 96, 144, 0, 48, 48);
        }
        im->drawSubRectInvertedOn(im, 0, 144, 0, 104, 320, 40);
        im->drawSubRectOn(im, 0, 184, 0, 96, 320, 8);
        borderInfo->image->alphaOn();

        delete borderInfo->image;
        borderInfo->image = NULL;
    }

    /* -----------------------------
     * draw "Lord British" signature
     * ----------------------------- */
    color = im->setColor(0, 255, 255);  // cyan for EGA
    int blue[16] = {255, 250, 226, 226, 210, 194, 161, 161,
                    129,  97,  97,  64,  64,  32,  32,   0};
    i = 0;
    while (sigData[i] != 0) {
        /* (x/y) are unscaled coordinates, i.e. in 320x200 */
        x = sigData[i] + 0x14;
        y = 0xBF - sigData[i+1];

        if (settings.videoType != "EGA")
        {
            // yellow gradient
            color = im->setColor(255, (y == 1 ? 250 : 255), blue[y]);
        }

        im->fillRect(x * prescale, y * prescale,
                     2 * prescale, prescale,
                     color.r, color.g, color.b);
        i += 2;
    }

    /* --------------------------------------------------------------
     * draw the red line between "Origin Systems, Inc." and "present"
     * -------------------------------------------------------------- */
    /* we're still working with an unscaled surface */
    if (settings.videoType != "EGA")
    {
        color = im->setColor(0, 0, 161);    // dark blue
    }
    else
    {
        color = im->setColor(128, 0, 0);    // dark red for EGA
    }
    for (i = 84; i < 236; i++)  // 152 px wide
        im->fillRect(i * prescale, 31 * prescale,
                     prescale, prescale,
                     color.r, color.g, color.b);
}

void ImageMgr::fixupAbyssVision(Image *im, int prescale) {
    static unsigned int *data = NULL;

    /*
     * Each VGA vision components must be XORed with all the previous
     * vision components to get the actual image.
     */
    if (data != NULL) {
        for (int y = 0; y < im->height(); y++) {
            for (int x = 0; x < im->width(); x++) {
                unsigned int index;
                im->getPixelIndex(x, y, index);
                index ^= data[y * im->width() + x];
                im->putPixelIndex(x, y, index);
            }
        }
    } else {
        data = new unsigned int[im->width() * im->height()];
    }

    for (int y = 0; y < im->height(); y++) {
        for (int x = 0; x < im->width(); x++) {
            unsigned int index;
            im->getPixelIndex(x, y, index);
            data[y * im->width() + x] = index;
        }
    }
}

void ImageMgr::fixupAbacus(Image *im, int prescale) {

    /* 
     * surround each bead with a row green pixels to avoid artifacts
     * when scaling
     */

    im->fillRect(7 * prescale, 186 * prescale, prescale, 14 * prescale, 0, 255, 80); /* green */
    im->fillRect(16 * prescale, 186 * prescale, prescale, 14 * prescale, 0, 255, 80); /* green */
    im->fillRect(8 * prescale, 186 * prescale, prescale * 8, prescale, 0, 255, 80); /* green */
    im->fillRect(8 * prescale, 199 * prescale, prescale * 8, prescale, 0, 255, 80); /* green */

    im->fillRect(23 * prescale, 186 * prescale, prescale, 14 * prescale, 0, 255, 80); /* green */
    im->fillRect(32 * prescale, 186 * prescale, prescale, 14 * prescale, 0, 255, 80); /* green */
    im->fillRect(24 * prescale, 186 * prescale, prescale * 8, prescale, 0, 255, 80); /* green */
    im->fillRect(24 * prescale, 199 * prescale, prescale * 8, prescale, 0, 255, 80); /* green */
}

/**
 * Swap blue and green for the dungeon walls when facing north or
 * south.
 */
void ImageMgr::fixupDungNS(Image *im, int prescale) {
    for (int y = 0; y < im->height(); y++) {
        for (int x = 0; x < im->width(); x++) {
            unsigned int index;
            im->getPixelIndex(x, y, index);
            if (index == 1)
                im->putPixelIndex(x, y, 2);
            else if (index == 2)
                im->putPixelIndex(x, y, 1);
        }
    }
}

/**
 * The FMTowns images have a different screen dimension. This moves them up to what xu4 is accustomed to.
 * south.
 */
void ImageMgr::fixupFMTowns(Image *im, int prescale) {
    for (int y = 20; y < im->height(); y++) {
        for (int x = 0; x < im->width(); x++) {
            unsigned int index;
            im->getPixelIndex(x, y, index);
			im->putPixelIndex(x, y-20, index);
        }
    }
}

/**
 * Returns information for the given image set.
 */
ImageSet *ImageMgr::getSet(const string &setname) {
    std::map<string, ImageSet *>::iterator i = imageSets.find(setname);
    if (i != imageSets.end())
        return i->second;
    else
        return NULL;
}

/**
 * Returns image information for the current image set.
 */
ImageInfo *ImageMgr::getInfo(const string &name) {
    return getInfoFromSet(name, baseSet);
}

/**
 * Returns information for the given image set.
 */
ImageInfo *ImageMgr::getInfoFromSet(const string &name, ImageSet *imageset) {
    if (!imageset)
        return NULL;

    /* if the image set contains the image we want, AND IT EXISTS we are done */
    std::map<string, ImageInfo *>::iterator i = imageset->info.find(name);
    if (i != imageset->info.end())
    	if (imageExists(i->second))
    		return i->second;

    /* otherwise if this image set extends another, check the base image set */
    while (imageset->extends != "") {
        imageset = getSet(imageset->extends);
        return getInfoFromSet(name, imageset);
    }

    //xu4_error(XU4_LOG_WRN, "Searched recursively from imageset %s through to %s and couldn't find %s", baseSet->name.c_str(), imageset->name.c_str(), name.c_str());
    return NULL;
}

std::string ImageMgr::guessFileType(const string &filename) {
    if (filename.length() >= 4 && filename.compare(filename.length() - 4, 4, ".png") == 0) {
        return "image/png";
    } else {
        return "";
    }
}

bool ImageMgr::imageExists(ImageInfo * info)
{
	if (info->filename == "") //If it is an abstract image like "screen"
		return true;
	U4FILE * file = getImageFile(info);
	if (file)
	{
		u4fclose(file);
		return true;
	}
	return false;
}


U4FILE * ImageMgr::getImageFile(ImageInfo *info)
{
	string filename = info->filename;

    /*
     * If the u4 VGA upgrade is installed (i.e. setup has been run and
     * the u4dos files have been renamed), we need to use VGA names
     * for EGA and vice versa, but *only* when the upgrade file has a
     * .old extention.  The charset and tiles have a .vga extention
     * and are not renamed in the upgrade installation process
     */
	if (u4isUpgradeInstalled() && getInfoFromSet(info->name, getSet("VGA"))->filename.find(".old") != string::npos) {
        if (settings.videoType == "EGA")
            filename = getInfoFromSet(info->name, getSet("VGA"))->filename;
        else
            filename = getInfoFromSet(info->name, getSet("EGA"))->filename;
    }

    if (filename == "")
    	return NULL;

    U4FILE *file = NULL;
    if (info->xu4Graphic) {
        string pathname(u4find_graphics(filename));

        if (!pathname.empty())
            file = u4fopen_stdio(pathname);
    }
    else {
        file = u4fopen(filename);
    }
    return file;
}

/**
 * Load in a background image from a ".ega" file.
 */
ImageInfo *ImageMgr::get(const string &name, bool returnUnscaled) {
    ImageInfo *info = getInfo(name);
    if (!info)
        return NULL;

    /* return if already loaded */
    if (info->image != NULL)
        return info;

    U4FILE *file = getImageFile(info);
    Image *unscaled = NULL;
    if (file) {
        TRACE(*logger, string("loading image from file '") + info->filename + string("'"));

        if (info->filetype.empty()) {
            info->filetype = guessFileType(info->filename);
		}
		
        string filetype = info->filetype;
		
        if (filetype == "image/png") {
			unscaled = xu4_png_load(info->filename.c_str(), &info->width, &info->height);
		}
        else {
			ImageLoader *loader = ImageLoader::getLoader(filetype);
			
			if (loader == NULL) {
				xu4_error(XU4_LOG_WRN, "can't find loader to load image \"%s\" with type \"%s\"", info->filename.c_str(), filetype.c_str());
			}
			else {
				unscaled = loader->load(file, info->width, info->height, info->depth);
				if (info->width == -1) {
					// Write in the values for later use.
					info->width = unscaled->width();
					info->height = unscaled->height();
					// ###            info->depth = ???
				}
			}
		}
        u4fclose(file);
    }
    else
    {
        xu4_error(XU4_LOG_WRN, "Failed to open file %s for reading.", info->filename.c_str());
        return NULL;
    }

    if (unscaled == NULL)
        return NULL;

    if (info->transparentIndex != -1)
        unscaled->setTransparentIndex(info->transparentIndex);

    if (info->prescale == 0)
        info->prescale = 1;

    /*
     * fixup the image before scaling it
     */
    switch (info->fixup) {
    case FIXUP_NONE:
        break;
    case FIXUP_INTRO:
        fixupIntro(unscaled, info->prescale);
        break;
    case FIXUP_ABYSS:
        fixupAbyssVision(unscaled, info->prescale);
        break;
    case FIXUP_ABACUS:
        fixupAbacus(unscaled, info->prescale);
        break;
    case FIXUP_DUNGNS:
        fixupDungNS(unscaled, info->prescale);
        break;
    case FIXUP_FMTOWNSSCREEN:
    	fixupFMTowns(unscaled, info->prescale);
    	break;
    case FIXUP_BLACKTRANSPARENCYHACK:
        //Apply transparency shadow hack to ultima4 ega and vga upgrade classic graphics.
    	Image *unscaled_original = unscaled;
    	unscaled = Image::duplicate(unscaled);
    	delete unscaled_original;
    	if (Settings::getInstance().enhancements && Settings::getInstance().enhancementsOptions.u4TileTransparencyHack)
    	{
    		int transparency_shadow_size =Settings::getInstance().enhancementsOptions.u4TrileTransparencyHackShadowBreadth;
    		int black_index = 0;
    		int opacity = Settings::getInstance().enhancementsOptions.u4TileTransparencyHackPixelShadowOpacity;

    		int frames = info->tiles;
    		for (int f = 0; f < frames; ++f)
    			unscaled->performTransparencyHack(black_index, frames, f, transparency_shadow_size, opacity);
    	}
        break;
    }

    if (returnUnscaled)
    {
        info->image = unscaled;
        return info;
    }

    int imageScale = settings.scale;
    if ((settings.scale % info->prescale) != 0) {
        int orig_scale = settings.scale;
        settings.scale = info->prescale;
        settings.write();
    	xu4_error(XU4_LOG_ERR, "image %s is prescaled to an incompatible size: %d\nResetting the scale to %d. Sorry about the inconvenience, please restart.", info->filename.c_str(), orig_scale, settings.scale);
    }
    imageScale /= info->prescale;

    info->image = screenScale(unscaled, imageScale, info->tiles, 1);

    delete unscaled;
    return info;
}

/**
 * Returns information for the given image set.
 */
SubImage *ImageMgr::getSubImage(const string &name) {
    string setname;

    ImageSet *set = baseSet;

    while (set != NULL) {
        for (std::map<string, ImageInfo *>::iterator i = set->info.begin(); i != set->info.end(); i++) {
            ImageInfo *info = (ImageInfo *) i->second;
            std::map<string, SubImage *>::iterator j = info->subImages.find(name);
            if (j != info->subImages.end())
                return j->second;
        }

        set = getSet(set->extends);
    }
        
    return NULL;
}

/**
 * Free up any background images used only in the animations.
 */
void ImageMgr::freeIntroBackgrounds() {
    for (std::map<string, ImageSet *>::iterator i = imageSets.begin(); i != imageSets.end(); i++) {
        ImageSet *set = i->second;
        for (std::map<string, ImageInfo *>::iterator j = set->info.begin(); j != set->info.end(); j++) {
            ImageInfo *info = j->second;
            if (info->image != NULL && info->introOnly) {
                delete info->image;
                info->image = NULL;
            }
        }
    }
}

const vector<string> &ImageMgr::getSetNames() {
    return imageSetNames;
}

/**
 * Find the new base image set when settings have changed.
 */
void ImageMgr::update(Settings *newSettings) {
    string setname;

    setname = newSettings->videoType;

    TRACE(*logger, string("base image set is '") + setname + string("'"));

    baseSet = getSet(setname);
}

ImageSet::~ImageSet() {
    for (std::map<string, ImageInfo *>::iterator i = info.begin(); i != info.end(); i++) {
        ImageInfo *imageInfo = i->second;
        if (imageInfo->name != "screen")
            delete imageInfo;
    }
}

ImageInfo::~ImageInfo() {
    for (std::map<string, SubImage *>::iterator i = subImages.begin(); i != subImages.end(); i++)
        delete i->second;
    if (image != NULL)
        delete image;
}
