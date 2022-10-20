/*
 * $Id: imagemgr.cpp 3048 2012-06-23 11:50:23Z twschulz $
 */

#include <vector>

#include "config.h"
#include "error.h"
#include "image.h"
#include "imageloader.h"
#include "imagemgr.h"
#include "intro.h"
#include "u4file.h"

bool ImageInfo::hasBlackBackground()
{
	return this->filetype == "image/x-u4raw";
}

struct ImageSet {
public:
    ~ImageSet();

    std::string name;
    std::string location;
    std::string extends;
    std::map<std::string, ImageInfo *> info;
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
    zu4_error(ZU4_LOG_DBG, "Creating ImageMgr");
    //settings.addObserver(this);
}

ImageMgr::~ImageMgr() {
    //settings.deleteObserver(this);
    for (std::map<std::string, ImageSet *>::iterator i = imageSets.begin(); i != imageSets.end(); i++)
        delete i->second;
}

void ImageMgr::init() {
    zu4_error(ZU4_LOG_DBG, "Initializing ImageMgr");
    /*
     * register the "screen" image representing the entire screen
     */
    Image *screen = zu4_img_create_screen();
    ImageInfo *screenInfo = new ImageInfo;

    screenInfo->name = "screen";
    screenInfo->filename = "";
    screenInfo->width = SCREEN_WIDTH;
    screenInfo->height = SCREEN_HEIGHT;
    screenInfo->depth = sizeof(uint32_t);
    screenInfo->filetype = "";
    screenInfo->tiles = 0;
    screenInfo->introOnly = false;
    screenInfo->xu4Graphic = false;
    screenInfo->fixup = FIXUP_NONE;
    screenInfo->image = screen;

    /*
     * register all the images declared in the config files
     */
    const Config *config = Config::getInstance();
    std::vector<ConfigElement> graphicsConf = config->getElement("graphics").getChildren();
    for (std::vector<ConfigElement>::iterator conf = graphicsConf.begin(); conf != graphicsConf.end(); conf++) {
        if (conf->getName() == "imageset") {
            ImageSet *set = loadImageSetFromConf(*conf);
            imageSets[set->name] = set;

            // all image sets include the "screen" image
            set->info[screenInfo->name] = screenInfo;
        }
    }

    imageSetNames.clear();
    for (std::map<std::string, ImageSet *>::const_iterator set = imageSets.begin(); set != imageSets.end(); set++)
        imageSetNames.push_back(set->first);

    update(&settings);
}

ImageSet *ImageMgr::loadImageSetFromConf(const ConfigElement &conf) {
    ImageSet *set;

    set = new ImageSet;
    set->name = conf.getString("name");
    set->location = conf.getString("location");
    set->extends = conf.getString("extends");

    zu4_error(ZU4_LOG_DBG, "Loading image set: %s", set->name.c_str());

    std::vector<ConfigElement> children = conf.getChildren();
    for (std::vector<ConfigElement>::iterator i = children.begin(); i != children.end(); i++) {
        if (i->getName() == "image") {
            ImageInfo *info = loadImageInfoFromConf(*i);
            std::map<std::string, ImageInfo *>::iterator dup = set->info.find(info->name);
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
    info->filetype = conf.getString("filetype");
    info->tiles = conf.getInt("tiles");
    info->introOnly = conf.getBool("introOnly");

    info->xu4Graphic = conf.getBool("xu4Graphic");
    info->fixup = static_cast<ImageFixup>(conf.getEnum("fixup", fixupEnumStrings));
    info->image = NULL;

    std::vector<ConfigElement> children = conf.getChildren();
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
    snprintf(subimage->name, sizeof(subimage->name), "%s", conf.getString("name").c_str());
    subimage->width = conf.getInt("width");
    subimage->height = conf.getInt("height");
    snprintf(subimage->srcImageName, sizeof(subimage->srcImageName), "%s", info->name.c_str());
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

void ImageMgr::fixupIntro(Image *im) {
	const unsigned char *sigData;
	int i, x, y;
	RGBA color;

	sigData = intro->getSigData();

	// update the position of "and"
	zu4_img_draw_subrect_on(im, im, 148, 17, 153, 17, 11, 4);
	zu4_img_draw_subrect_on(im, im, 159, 17, 165, 18, 1, 4);
	zu4_img_draw_subrect_on(im, im, 160, 17, 164, 17, 16, 4);

	// update the position of "Origin Systems, Inc."
	//zu4_img_draw_subrect_on(im, im, 86, 21, 88, 21, 114, 9);
	//zu4_img_draw_subrect_on(im, im, 199, 21, 202, 21, 6, 9);
	zu4_img_draw_subrect_on(im, im, 207, 21, 208, 21, 28, 9);

	// update the position of "Ultima IV" -  move this prior to moving "present"
	zu4_img_draw_subrect_on(im, im, 59, 33, 61, 33, 204, 46);

	// update the position of "Quest of the Avatar"
	//zu4_img_draw_subrect_on(im, im, 69, 80, 70, 80, 11, 13); // quEst
	//zu4_img_draw_subrect_on(im, im, 82, 80, 84, 80, 27, 13); // queST
	//zu4_img_draw_subrect_on(im, im, 131, 80, 132, 80, 11, 13); // oF
	//zu4_img_draw_subrect_on(im, im, 150, 80, 149, 80, 40, 13); // THE
	//zu4_img_draw_subrect_on(im, im, 166, 80, 165, 80, 11, 13); // tHe
	zu4_img_draw_subrect_on(im, im, 200, 80, 201, 80, 81, 13); // AVATAR
	//zu4_img_draw_subrect_on(im, im, 227, 80, 228, 80, 11, 13); // avAtar

	// copy "present" to new location between "Origin Systems, Inc." and
	// "Ultima IV" - do this after moving "Ultima IV"
	zu4_img_draw_subrect_on(im, im, 132, 33, 135, 0, 56,5);

	// erase the original "present"
	zu4_img_fill(im, 135, 0, 56, 5, 0, 0, 0, 255);

	// update the colors for VGA
	if (settings.videoType == 1) { // VGA
		ImageInfo *borderInfo = imageMgr->get(BKGD_BORDERS, true);
		if (!borderInfo)
			zu4_error(ZU4_LOG_ERR, "Unable to load the \"%s\" data file.\n", BKGD_BORDERS);

		zu4_img_free(borderInfo->image);
		borderInfo->image = NULL;
		borderInfo = imageMgr->get(BKGD_BORDERS, true);

		// update the border appearance
		zu4_img_draw_subrect_on(im, borderInfo->image, 0, 96, 0, 0, 16, 56);
		for (int i=0; i < 9; i++) {
			zu4_img_draw_subrect_on(im, borderInfo->image, 16+(i*32), 96, 144, 0, 48, 48);
		}
		zu4_img_draw_subrect_inv(im, im, 0, 144, 0, 104, 320, 40);
		zu4_img_draw_subrect_on(im, im, 0, 184, 0, 96, 320, 8);

		zu4_img_free(borderInfo->image);
		borderInfo->image = NULL;
	}

	// draw "Lord British" signature
	color = RGBA{0, 255, 255, 255};  // cyan for EGA
	uint8_t blue[16] = {
		255, 250, 226, 226, 210, 194, 161, 161,
		129, 97, 97, 64, 64, 32, 32, 0
	};

	i = 0;
	while (sigData[i] != 0) {
		x = sigData[i] + 0x14;
		y = 0xBF - sigData[i+1];

		if (settings.videoType) { // Not EGA
			// yellow gradient
			color = RGBA{255, (uint8_t)(y == 1 ? 250 : 255), blue[y], 255};
		}

		zu4_img_fill(im, x, y, 2, 1, color.r, color.g, color.b, 255);
		i += 2;
	}

	// draw the red line between "Origin Systems, Inc." and "present"
	if (settings.videoType) { // Not EGA
		color = RGBA{0, 0, 161, 255}; // dark blue
	}
	else {
		color = RGBA{128, 0, 0, 255}; // dark red for EGA
	}

	for (i = 84; i < 236; i++) { // 152 px wide
		zu4_img_fill(im, i, 31, 1, 1, color.r, color.g, color.b, 255);
	}
}

void ImageMgr::fixupAbyssVision(Image *im) {
    static unsigned int *data = NULL;

    /*
     * Each VGA vision components must be XORed with all the previous
     * vision components to get the actual image.
     */
    if (data != NULL) {
        for (int y = 0; y < im->h; y++) {
            for (int x = 0; x < im->w; x++) {
                uint32_t index = zu4_img_get_pixel(im, x, y);
                index ^= data[y * im->w + x];
                zu4_img_set_pixel(im, x, y, index);
            }
        }
    } else {
        data = new unsigned int[im->w * im->h];
    }

    for (int y = 0; y < im->h; y++) {
        for (int x = 0; x < im->w; x++) {
            uint32_t index = zu4_img_get_pixel(im, x, y);
            data[y * im->w + x] = index;
        }
    }
}

/**
 * Swap blue and green for the dungeon walls when facing north or
 * south.
 */
void ImageMgr::fixupDungNS(Image *im) {
    for (int y = 0; y < im->h; y++) {
        for (int x = 0; x < im->w; x++) {
            uint32_t index = zu4_img_get_pixel(im, x, y);
            if (index == 1)
                zu4_img_set_pixel(im, x, y, 2);
            else if (index == 2)
                zu4_img_set_pixel(im, x, y, 1);
        }
    }
}

/**
 * Returns information for the given image set.
 */
ImageSet *ImageMgr::getSet(const std::string &setname) {
    std::map<std::string, ImageSet *>::iterator i = imageSets.find(setname);
    if (i != imageSets.end())
        return i->second;
    else
        return NULL;
}

/**
 * Returns image information for the current image set.
 */
ImageInfo *ImageMgr::getInfo(const std::string &name) {
    return getInfoFromSet(name, baseSet);
}

/**
 * Returns information for the given image set.
 */
ImageInfo *ImageMgr::getInfoFromSet(const std::string &name, ImageSet *imageset) {
    if (!imageset)
        return NULL;

    /* if the image set contains the image we want, AND IT EXISTS we are done */
    std::map<std::string, ImageInfo *>::iterator i = imageset->info.find(name);
    if (i != imageset->info.end())
    	if (imageExists(i->second))
    		return i->second;

    /* otherwise if this image set extends another, check the base image set */
    while (imageset->extends != "") {
        imageset = getSet(imageset->extends);
        return getInfoFromSet(name, imageset);
    }

    //zu4_error(ZU4_LOG_WRN, "Searched recursively from imageset %s through to %s and couldn't find %s", baseSet->name.c_str(), imageset->name.c_str(), name.c_str());
    return NULL;
}

std::string ImageMgr::guessFileType(const std::string &filename) {
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
		if (info->xu4Graphic) zu4_file_stdio_close(file);
		else u4fclose(file);
		return true;
	}
	return false;
}

U4FILE * ImageMgr::getImageFile(ImageInfo *info)
{
	std::string filename = info->filename;

    /*
     * If the u4 VGA upgrade is installed (i.e. setup has been run and
     * the u4dos files have been renamed), we need to use VGA names
     * for EGA and vice versa, but *only* when the upgrade file has a
     * .old extention.  The charset and tiles have a .vga extention
     * and are not renamed in the upgrade installation process
     */
	if (u4isUpgradeInstalled() && getInfoFromSet(info->name, getSet("VGA"))->filename.find(".old") != std::string::npos) {
        if (!settings.videoType) // EGA
            filename = getInfoFromSet(info->name, getSet("VGA"))->filename;
        else
            filename = getInfoFromSet(info->name, getSet("EGA"))->filename;
    }

    if (filename == "")
    	return NULL;

    U4FILE *file = NULL;
    if (info->xu4Graphic) {
        char path[64];
        u4find_graphics(path, sizeof(path), filename.c_str());
        std::string pathname = (std::string)path;

        if (!pathname.empty())
            file = u4fopen_stdio(path);
    }
    else {
        file = u4fopen(filename.c_str());
    }
    return file;
}

/**
 * Load in a background image from a ".ega" file.
 */
ImageInfo *ImageMgr::get(const std::string &name, bool returnUnscaled) {
    ImageInfo *info = getInfo(name);
    if (!info)
        return NULL;

    /* return if already loaded */
    if (info->image != NULL)
        return info;

    U4FILE *file = getImageFile(info);
    Image *unscaled = NULL;
    if (file) {
        //zu4_error(ZU4_LOG_DBG, "Loading image from file: %s", info->filename.c_str());

        if (info->filetype.empty()) {
            info->filetype = guessFileType(info->filename);
		}

        std::string filetype = info->filetype;
        int imgtype = 0;

        if (filetype == "image/png") { imgtype = ZU4_IMG_PNG; }
        else if (filetype == "image/x-u4raw") { imgtype = ZU4_IMG_RAW; }
        else if (filetype == "image/x-u4rle") { imgtype = ZU4_IMG_RLE; }
        else if (filetype == "image/x-u4lzw") { imgtype = ZU4_IMG_LZW; }
        else {
			zu4_error(ZU4_LOG_WRN, "can't find loader to load image \"%s\" with type \"%s\"", info->filename.c_str(), filetype.c_str());
		}

		switch(imgtype) {
			case ZU4_IMG_RAW: case ZU4_IMG_RLE: case ZU4_IMG_LZW:
				unscaled = zu4_img_load(file, info->width, info->height, info->depth, imgtype);
				if (info->width == -1) {
					info->width = unscaled->w;
					info->height = unscaled->h;
				}
				break;

			case ZU4_IMG_PNG:
				char imgpath[64];
				u4find_graphics(imgpath, sizeof(imgpath), info->filename.c_str());
				unscaled = zu4_png_load(imgpath, &info->width, &info->height);
				break;
			default: break;
		}
        if (info->xu4Graphic) zu4_file_stdio_close(file);
		else u4fclose(file);
    }
    else
    {
        zu4_error(ZU4_LOG_WRN, "Failed to open file %s for reading.", info->filename.c_str());
        return NULL;
    }

    if (unscaled == NULL)
        return NULL;

    /*
     * fixup the image before scaling it
     */
    switch (info->fixup) {
    case FIXUP_NONE:
        break;
    case FIXUP_INTRO:
        fixupIntro(unscaled);
        break;
    case FIXUP_ABYSS:
        fixupAbyssVision(unscaled);
        break;
    case FIXUP_DUNGNS:
        fixupDungNS(unscaled);
        break;
    }

    info->image = unscaled;
    return info;
}

/**
 * Returns information for the given image set.
 */
SubImage *ImageMgr::getSubImage(const std::string &name) {
    std::string setname;

    ImageSet *set = baseSet;

    while (set != NULL) {
        for (std::map<std::string, ImageInfo *>::iterator i = set->info.begin(); i != set->info.end(); i++) {
            ImageInfo *info = (ImageInfo *) i->second;
            std::map<std::string, SubImage *>::iterator j = info->subImages.find(name);
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
    for (std::map<std::string, ImageSet *>::iterator i = imageSets.begin(); i != imageSets.end(); i++) {
        ImageSet *set = i->second;
        for (std::map<std::string, ImageInfo *>::iterator j = set->info.begin(); j != set->info.end(); j++) {
            ImageInfo *info = j->second;
            if (info->image != NULL && info->introOnly) {
                zu4_img_free(info->image);
                info->image = NULL;
            }
        }
    }
}

const std::vector<std::string> &ImageMgr::getSetNames() {
    return imageSetNames;
}

/**
 * Find the new base image set when settings have changed.
 */
void ImageMgr::update(SettingsData *newSettings) {
    std::string setname = newSettings->videoType ? "VGA" : "EGA";//newSettings->videoType;
    zu4_error(ZU4_LOG_DBG, "Base image set is: %s", setname.c_str());
    baseSet = getSet(setname);
}

ImageSet::~ImageSet() {
    for (std::map<std::string, ImageInfo *>::iterator i = info.begin(); i != info.end(); i++) {
        ImageInfo *imageInfo = i->second;
        if (imageInfo->name != "screen")
            delete imageInfo;
    }
}

ImageInfo::~ImageInfo() {
    for (std::map<std::string, SubImage *>::iterator i = subImages.begin(); i != subImages.end(); i++)
        delete i->second;
    if (image != NULL)
        zu4_img_free(image);
}
