#include<cstring>
#include<cstdio>
#include"httpdata.h"
#include"rtspdata.h"
#include"shareheader.h"
#include"httputil.h"
#include"tokenutil.h"
#include"cache.h"
#include"misc.h"
#include"rtspsdp.h"
#include"avcodec.h"


static const int DEF_RTP_PROTOCOL_CNT = 3;
static const char* DEF_RTP_PROTOCOL[DEF_RTP_PROTOCOL_CNT] = {
    "RTP", "AVP", "UDP"
};

#define RTSP_DEFAULT_AUDIO_SAMPLERATE 44100
#define RTSP_DEFAULT_NB_AUDIO_CHANNELS 1

static const struct {
    int pt;
    const char enc_name[32];
    enum AVMediaType codec_type;
    enum AVCodecID codec_id;
    int clock_rate;
    int audio_channels;
} rtp_payload_types[] = {
  {3, "GSM",         AVMEDIA_TYPE_AUDIO,   AV_CODEC_ID_NONE, 8000, 1},
  {4, "G723",        AVMEDIA_TYPE_AUDIO,   AV_CODEC_ID_G723_1, 8000, 1},
  {5, "DVI4",        AVMEDIA_TYPE_AUDIO,   AV_CODEC_ID_NONE, 8000, 1},
  {6, "DVI4",        AVMEDIA_TYPE_AUDIO,   AV_CODEC_ID_NONE, 16000, 1},
  {7, "LPC",         AVMEDIA_TYPE_AUDIO,   AV_CODEC_ID_NONE, 8000, 1},
  {12, "QCELP",      AVMEDIA_TYPE_AUDIO,   AV_CODEC_ID_QCELP, 8000, 1},
  {13, "CN",         AVMEDIA_TYPE_AUDIO,   AV_CODEC_ID_NONE, 8000, 1},
  {14, "MPA",        AVMEDIA_TYPE_AUDIO,   AV_CODEC_ID_MP2, -1, -1},
  {14, "MPA",        AVMEDIA_TYPE_AUDIO,   AV_CODEC_ID_MP3, -1, -1},
  {15, "G728",       AVMEDIA_TYPE_AUDIO,   AV_CODEC_ID_NONE, 8000, 1},
  {16, "DVI4",       AVMEDIA_TYPE_AUDIO,   AV_CODEC_ID_NONE, 11025, 1},
  {17, "DVI4",       AVMEDIA_TYPE_AUDIO,   AV_CODEC_ID_NONE, 22050, 1},
  {18, "G729",       AVMEDIA_TYPE_AUDIO,   AV_CODEC_ID_NONE, 8000, 1},
  {25, "CelB",       AVMEDIA_TYPE_VIDEO,   AV_CODEC_ID_NONE, 90000, -1},
  {26, "JPEG",       AVMEDIA_TYPE_VIDEO,   AV_CODEC_ID_MJPEG, 90000, -1},
  {28, "nv",         AVMEDIA_TYPE_VIDEO,   AV_CODEC_ID_NONE, 90000, -1},
  {31, "H261",       AVMEDIA_TYPE_VIDEO,   AV_CODEC_ID_H261, 90000, -1},
  {32, "MPV",        AVMEDIA_TYPE_VIDEO,   AV_CODEC_ID_MPEG1VIDEO, 90000, -1},
  {32, "MPV",        AVMEDIA_TYPE_VIDEO,   AV_CODEC_ID_MPEG2VIDEO, 90000, -1},
  {34, "H263",       AVMEDIA_TYPE_VIDEO,   AV_CODEC_ID_H263, 90000, -1},

  {0, "X-MP3-draft-00", AVMEDIA_TYPE_AUDIO, AV_CODEC_ID_MP3ADU, -1, -1},
  {0, "speex", AVMEDIA_TYPE_AUDIO, AV_CODEC_ID_SPEEX, -1, -1},
  {0, "opus", AVMEDIA_TYPE_AUDIO, AV_CODEC_ID_OPUS, -1, -1},
  {0, "AMR", AVMEDIA_TYPE_AUDIO, AV_CODEC_ID_AMR_NB, -1, -1},
  {0, "AMR-WB", AVMEDIA_TYPE_AUDIO, AV_CODEC_ID_AMR_WB, -1, -1},
  {0, "H263-1998", AVMEDIA_TYPE_VIDEO, AV_CODEC_ID_H263, -1, -1},
  {0, "H263-2000", AVMEDIA_TYPE_VIDEO, AV_CODEC_ID_H263, -1, -1},
  {0, "H264",       AVMEDIA_TYPE_VIDEO,   AV_CODEC_ID_H264, 90000, -1}, 
  {0, "iLBC", AVMEDIA_TYPE_AUDIO, AV_CODEC_ID_ILBC, -1, -1},
  {0, "JPEG", AVMEDIA_TYPE_VIDEO, AV_CODEC_ID_MJPEG, -1, -1},
  {0, "MP4A-LATM", AVMEDIA_TYPE_AUDIO, AV_CODEC_ID_AAC, -1, -1},
  {0, "MP4V-ES", AVMEDIA_TYPE_VIDEO, AV_CODEC_ID_MPEG4, -1, -1},
  {0, "MPEG4-GENERIC", AVMEDIA_TYPE_AUDIO, AV_CODEC_ID_AAC, 44100, 2},
  {0, "x-Purevoice", AVMEDIA_TYPE_AUDIO, AV_CODEC_ID_QCELP, -1, -1},
  {0, "X-QDM", AVMEDIA_TYPE_AUDIO, AV_CODEC_ID_NONE, -1, -1},
  {0, "X-SV3V-ES",AVMEDIA_TYPE_VIDEO, AV_CODEC_ID_NONE, -1, -1},
  {0, "VP8", AVMEDIA_TYPE_VIDEO, AV_CODEC_ID_VP8, -1, -1},
  {0, "theora", AVMEDIA_TYPE_VIDEO, AV_CODEC_ID_THEORA, -1, -1},
  {0, "vorbis", AVMEDIA_TYPE_AUDIO, AV_CODEC_ID_VORBIS, -1, -1},
    
  {-1, "",           AVMEDIA_TYPE_UNKNOWN, AV_CODEC_ID_NONE, -1, -1}
};

SdpData* SdpUtil::creatSDP(const Token& path) {
    SdpData* sdp = NULL;
    SdpHeader* header = NULL;

    sdp = (SdpData*)CacheUtil::mallocAlign(sizeof(SdpData));
    MiscTool::bzero(sdp, sizeof(SdpData));
    
    header = &sdp->m_header;
    TokenUtil::copy(header->m_path, sizeof(header->m_path), &path);
 
    return sdp;
}

void SdpUtil::freeSDP(SdpData* sdp) {
    if (NULL != sdp) {
        for (int i=0; i<sdp->m_media_cnt; ++i) {
            freeMedia(sdp->m_media[i]);
            sdp->m_media[i] = NULL;
        }
        
        CacheUtil::freeAlign(sdp);
    }
}

SdpMedia* SdpUtil::creatMedia() {
    SdpMedia* media = NULL;

    media = (SdpMedia*)CacheUtil::mallocAlign(sizeof(SdpMedia));
    MiscTool::bzero(media, sizeof(SdpMedia));
    return media;
}

void SdpUtil::freeMedia(SdpMedia* media) {
    if (NULL != media) {
        HttpUtil::release(&media->m_codec.m_config);
        CacheUtil::freeAlign(media);
    }
}

int SdpUtil::findMedia(SdpData* sdp, const Token& path) {
    SdpHeader* header = NULL;
    SdpMedia* media = NULL;
    Token dst;
    Token needle;

    header = &sdp->m_header;
    dst.set(header->m_path);

    do {
        
        if (dst.m_len > path.m_len) {
            break;
        }
        
        needle.set(path.m_str, dst.m_len); 
        if (!TokenUtil::strCmp(&dst, needle, false)) {
            break;
        }

        if (dst.m_len == path.m_len) {
            /* match resource path */
            return 0;
        } else if (DEF_URL_SEP_CHAR != path.m_str[dst.m_len]) {
            break;
        }
        
        /* partial match */
        TokenUtil::substr(&needle, &path, dst.m_len + 1);
        if (TokenUtil::isEmpty(&needle)) {
            break;
        } 

        for (int i=0; i<sdp->m_media_cnt; ++i) {
            media = sdp->m_media[i];
            dst.set(media->m_path);
            if (TokenUtil::strCmp(&dst, needle, false)) {
                return i;
            }
        }
    } while (false);
    
    return -1;
}

int SdpUtil::prepareSDP(HttpCache* cache, const SdpData* sdp) {
    const SdpHeader* header = NULL;
    const SdpMedia* media = NULL;
    int total = 0; 

    header = &sdp->m_header;
    total = writeSdpHeader(cache, header); 
    
    for (int i=0; i<sdp->m_media_cnt; ++i) {
        media = sdp->m_media[i];
        total += writeSdpMedia(cache, media); 
    }

    return total;
}

int SdpUtil::writeSdpAddr(HttpCache* cache, const char ip[], int ttl) {
    int total = 0;
    
    if (0 < ttl) {
        total = HttpUtil::addFormat(cache,
            "c=IN IP4 %s/%d\r\n", ip, ttl);
    } else {
        total = HttpUtil::addFormat(cache,
            "c=IN IP4 %s\r\n", ip); 
    }

    return total;
}

int SdpUtil::writeSdpHeader(HttpCache* cache, 
    const SdpHeader* header) { 
    int total = 0;
    
    total += HttpUtil::addFormat(cache,
        "v=%d\r\n"
        "o=- %d %d IN %s %s\r\n"
        "s=%s\r\n",
        header->m_version, 
        0, 0, "IP4", "127.0.0.1",
        DEF_NULL_CHAR != header->m_title[0] ?
        header->m_title : "No Name");

    total += writeSdpAddr(cache, "0.0.0.0", 0);

    total += HttpUtil::addFormat(cache,
        "t=0 0\r\n"
        "a=tool:libavformat 57.25.100\r\n");
    
    return total;
}

int SdpUtil::getPayloadType(const SdpMedia* media) {
    return RTP_PT_PRIVATE + media->m_idx;
}

int SdpUtil::getProfileLevel(const SdpMedia* media) {
    const AVCodec* codec = &media->m_codec;
    int profile_level = 0x2B;

    if (codec->m_sample_rate <= 24000) {
        if (codec->m_channels <= 2)
            profile_level = 0x28; // AAC Profile, Level 1
    } else if (codec->m_sample_rate <= 48000) {
        if (codec->m_channels <= 2) {
            profile_level = 0x29; // AAC Profile, Level 2
        } else if (codec->m_channels <= 5) {
            profile_level = 0x2A; // AAC Profile, Level 4
        }
    } else if (codec->m_sample_rate <= 96000) {
        if (codec->m_channels <= 5) {
            profile_level = 0x2B; // AAC Profile, Level 5
        }
    }

    return profile_level;
}

int SdpUtil::writeMediaType(HttpCache* cache, 
    const SdpMedia* media) {
    const AVCodec* codec = NULL;
    const char* codeType = NULL;
    int total = 0; 
    int port = 0;

    codec = &media->m_codec;
    
    switch (codec->m_codec_type) {
    case AVMEDIA_TYPE_VIDEO: 
        codeType = "video";
        break;
    case AVMEDIA_TYPE_AUDIO: 
        codeType = "audio"; 
        break;
    case AVMEDIA_TYPE_SUBTITLE: 
        codeType = "text"; 
        break;
    default: 
        codeType = "application";
        break;
    }

    total += HttpUtil::addFormat(cache,
        "m=%s %d RTP/AVP %d\r\n",
        codeType, port, 
        codec->m_payload);

    return total;
}

int SdpUtil::writeMediaAttr(HttpCache* cache, 
    const SdpMedia* media) {
    const AVCodec* codec = NULL;
    int total = 0; 

    codec = &media->m_codec; 

    if (AVMEDIA_TYPE_VIDEO == codec->m_codec_type) {
        total += HttpUtil::addFormat(cache,
            "a=rtpmap:%d %s/%d\r\n",
            codec->m_payload, codec->m_enc_name,
            codec->m_sample_rate);
    } else if (AVMEDIA_TYPE_AUDIO == codec->m_codec_type){
        total += HttpUtil::addFormat(cache,
            "a=rtpmap:%d %s/%d/%d\r\n",
            codec->m_payload, codec->m_enc_name,
            codec->m_sample_rate,
            codec->m_channels);
    }

    if (0 < codec->m_width && 0 < codec->m_height) {
        total += HttpUtil::addFormat(cache,
            "a=framesize:%d %d-%d\r\n",
            codec->m_payload, 
            codec->m_width,
            codec->m_height);
    }

    if (0 < codec->m_config.m_size) {
        total += HttpUtil::addFormat(cache,
            "a=fmtp:%d %.*s\r\n",
            codec->m_payload, 
            codec->m_config.m_size,
            HttpUtil::data(&codec->m_config));
    }

    return total;
}

int SdpUtil::writeSdpMedia(HttpCache* cache, 
    const SdpMedia* media) {
    const AVCodec* codec = NULL;
    int total = 0;

    codec = &media->m_codec;
    
    total += writeMediaType(cache, media); 
    if (0 < media->m_ttl) {
        total += writeSdpAddr(cache, 
            media->m_ip, media->m_ttl); 
    }
    
    if (0 < codec->m_bit_rate) {
        total += HttpUtil::addFormat(cache,
            "b=AS:%d\r\n",
            codec->m_bit_rate / 1000);
    }

    total += writeMediaAttr(cache, media); 
    total += HttpUtil::addFormat(cache,
        "a=control:%s\r\n", media->m_path); 
    
    return total;
}

int SdpUtil::parseSDP(SdpData* sdp, Token* text) {
    bool bOk = false;
    Token line;
    Token key;
    Token val;
    SdpParseStat stat;
    
    MiscTool::bzero(&stat, sizeof(stat));

    bOk = TokenUtil::next(&line, text, "\r\n");
    while (bOk) {
        bOk = TokenUtil::split(&key, &val, 
            &line, DEF_EQUATION_CHAR);
        if (bOk && 1 == key.m_len) { 
            parseSdpLine(sdp, stat, key.m_str[0], val);
        }

        /* next line */
        bOk = TokenUtil::next(&line, text, "\r\n");
    }

    if (0 < sdp->m_media_cnt) {
        return 0;
    } else {
        return -1;
    }
}

void SdpUtil::parseSdpLine(SdpData* sdp, 
    SdpParseStat& stat, char letter, Token& val) {

    if (stat.m_skip_media && 'm' != letter) {
        return;
    }
    
    if ('m' == letter) {
        parseMedia(sdp, stat, val);
    } else if ('c' == letter) {
        parseConn(sdp, stat, val);
    } else if ('a' == letter) {
        parseAttr(sdp, stat, val);
    } else if ('b' == letter) {
        parseBitRate(sdp, val);
    } else if ('s' == letter) {
        parseTitle(&sdp->m_header, val);
    } else if ('i' == letter) {
        parseDesc(&sdp->m_header, val); 
    } else {
    } 
}

void SdpUtil::parseConn(SdpData* sdp, 
    SdpParseStat& stat, Token& txt) {
    SdpMedia* media = NULL;
    Token val;
    Token ip;
    int ttl;

    do {
        TokenUtil::next(&val, &txt, " ");
        if (!TokenUtil::strCmp(&val, "IN", false)) {
            break;
        }

        TokenUtil::next(&val, &txt, " ");
        if (!TokenUtil::strCmp(&val, "IP4", false)) {
            break;
        }

        if (TokenUtil::next(&val, &txt, "/")) {
            ip = val;
        } else {
            break;
        }
        
        if (!TokenUtil::next(&val, &txt, "/")) {
            TokenUtil::toNum(&ttl, &val);
        } else {
            break;
        } 

        if (0 < sdp->m_media_cnt) {
            media = sdp->m_media[sdp->m_media_cnt - 1];

            TokenUtil::copy(media->m_ip, 
                sizeof(media->m_ip), &ip); 
            media->m_ttl = ttl;
        } else {
            TokenUtil::copy(stat.m_ip, 
                sizeof(stat.m_ip), &ip);
            stat.m_ttl = ttl;
        } 
    } while (false);
}

void SdpUtil::parseBitRate(SdpData* sdp, Token& txt) {
    SdpMedia* media = NULL;
    AVCodec* codec = NULL;
    Token key;
    Token val;
    int n = 0;

    do {
        if (!TokenUtil::split(&key, &val, 
            &txt, DEF_COLON_CHAR)) {
            break;
        }

        if (!TokenUtil::strCmp(&key, "AS", false)) { 
            break;
        }

        if (0 < sdp->m_media_cnt) {
            media = sdp->m_media[sdp->m_media_cnt-1];
            codec = &media->m_codec; 
        } else {
            break;
        }

        TokenUtil::toNum(&n, &val);
        codec->m_bit_rate = n * 1000;
    } while (false);
}

void SdpUtil::parseTitle(SdpHeader* header, Token& txt) {
    TokenUtil::copy(header->m_title, 
        sizeof(header->m_title), &txt);
}

void SdpUtil::parseDesc(SdpHeader* header, Token& txt) {
    TokenUtil::copy(header->m_comment,
        sizeof(header->m_comment), &txt);
}

void SdpUtil::parseAttr(SdpData* sdp, 
    SdpParseStat& stat, Token& txt) {
    Token key;
    Token val; 

    do {
        if (!TokenUtil::split(&key, &val,
            &txt, DEF_COLON_CHAR)) {
            break;
        }

        if (TokenUtil::strCmp(&key, "control", false)) { 
            parseControl(sdp, stat, val);
        } else if (TokenUtil::strCmp(&key, "rtpmap", false)) {
            parseRtpmap(sdp, stat, val);
        } else if (TokenUtil::strCmp(&key, "fmtp", false)) {
            parseFmtp(sdp, stat, val);
        } else if (TokenUtil::strCmp(&key, "framesize", false)) {
            parseFrameSize(sdp, stat, val);
        } else {
        }
    } while (false);
}

void SdpUtil::parseControl(SdpData* sdp, 
    SdpParseStat&, Token& txt) {
    SdpHeader* header = NULL;
    SdpMedia* media = NULL;
    Token schema;
    Token path;
    Token tmp;

    HttpUtil::splitUrl(&txt, &schema, NULL,
        NULL, NULL, &path);

    if (0 < sdp->m_media_cnt) {
        media = sdp->m_media[sdp->m_media_cnt - 1];
        
        if (!TokenUtil::isEmpty(&schema)) {
            TokenUtil::copy(media->m_url, sizeof(media->m_url), &txt);
        } else {
            TokenUtil::copy(media->m_path, sizeof(media->m_path), &path);
        } 
    } else {
        header = &sdp->m_header;
        
        if (!TokenUtil::isEmpty(&schema)) {
            TokenUtil::copy(header->m_url, sizeof(header->m_url), &txt);
        }
    }
}

void SdpUtil::parseRtpmap(SdpData* sdp, 
    SdpParseStat&, Token& txt) {
    SdpMedia* media = NULL;
    AVCodec* codec = NULL;
    int n = 0;
    Token tmp;
    bool bOk = false;
    
    do {
        if (!(0 < sdp->m_media_cnt)) {
            break;
        }

        media = sdp->m_media[sdp->m_media_cnt - 1]; 
        codec = &media->m_codec;
    
        bOk = TokenUtil::nextInt(&codec->m_payload, &txt, " "); 
        if (!bOk) {
            break;
        }

        bOk = TokenUtil::next(&tmp, &txt, "/"); 
        if (!bOk) {
            break;
        }

        TokenUtil::copy(codec->m_enc_name, sizeof(codec->m_enc_name), &tmp);
        getCodecID(codec, codec->m_enc_name);

        if (AVMEDIA_TYPE_AUDIO == codec->m_codec_type) {
            codec->m_sample_rate = RTSP_DEFAULT_AUDIO_SAMPLERATE;
            codec->m_channels = RTSP_DEFAULT_NB_AUDIO_CHANNELS;
            
            bOk = TokenUtil::nextInt(&n, &txt, "/"); 
            if (!bOk) {
                break;
            }

            if (0 < n) {
                codec->m_sample_rate = n;
            }

            bOk = TokenUtil::nextInt(&n, &txt, "/"); 
            if (!bOk) {
                break;
            }

            if (0 < n) {
                codec->m_channels = n;
            }
        } else if (AVMEDIA_TYPE_VIDEO == codec->m_codec_type) {
            bOk = TokenUtil::nextInt(&n, &txt, "/"); 
            if (!bOk) {
                break;
            } 

            if (0 < n) {
                codec->m_sample_rate = n;
            }
        } else {
            break;
        }
    } while (false);
}

void SdpUtil::parseFmtp(SdpData* sdp, 
    SdpParseStat&, Token& txt) {
    SdpMedia* media = NULL;
    AVCodec* codec = NULL;
    int payload = 0;
    bool bOk = false;
    Token needle;
    Token key;
    Token val;
    int cnt = 0;
    
    do {
        if (!(0 < sdp->m_media_cnt)) {
            break;
        }

        media = sdp->m_media[sdp->m_media_cnt - 1];
        codec = &media->m_codec;
        
        bOk = TokenUtil::nextInt(&payload, &txt, " "); 
        if (!bOk) {
            break;
        }

        if (codec->m_payload != payload) {
            break;
        }

        while (TokenUtil::next(&needle, &txt, ";")) {
            bOk = TokenUtil::split(&key, &val, &needle, 
                DEF_EQUATION_CHAR);
            if (bOk) {
                if (0 < cnt) {
                    HttpUtil::addChar(&codec->m_config,
                        DEF_SEMICOLON_CHAR);
                }
                
                HttpUtil::addToken(&codec->m_config, key);
                HttpUtil::addChar(&codec->m_config,
                    DEF_EQUATION_CHAR);
                HttpUtil::addToken(&codec->m_config, val);
                ++cnt;
            }
        }
    } while (false);
}

void SdpUtil::parseFrameSize(SdpData* sdp, 
    SdpParseStat&, Token& txt) {
    SdpMedia* media = NULL;
    AVCodec* codec = NULL;
    int payload = 0;
    bool bOk = false;
    
    do {
        if (!(0 < sdp->m_media_cnt)) {
            break;
        }

        media = sdp->m_media[sdp->m_media_cnt - 1];
        codec = &media->m_codec;
        
        bOk = TokenUtil::nextInt(&payload, &txt, " "); 
        if (!bOk) {
            break;
        }

        if (codec->m_payload != payload) {
            break;
        }

        bOk = TokenUtil::nextInt(&codec->m_width, &txt, "-"); 
        if (!bOk) {
            break;
        }

        bOk = TokenUtil::nextInt(&codec->m_height, &txt, "-"); 
        if (!bOk) {
            break;
        }
    } while (false);
}

void SdpUtil::postMedia(SdpMedia* media) {
    if (DEF_NULL_CHAR == media->m_path[0] &&
        DEF_NULL_CHAR != media->m_url[0]) {
        Token url(media->m_url);
        Token path;
        
        HttpUtil::splitUrl(&url, NULL, NULL,
            NULL, NULL, &path);
        TokenUtil::copy(media->m_path,
            sizeof(media->m_path), &path);
    }
}

void SdpUtil::parseMedia(SdpData* sdp, 
    SdpParseStat& stat, Token& txt) {
    SdpMedia* media = NULL;
    SdpHeader* header = NULL;
    AVCodec* codec = NULL;
    bool bOk = false;
    Token tmp;
    Token val;

    do {
        if (0 < sdp->m_media_cnt) {
            postMedia(sdp->m_media[sdp->m_media_cnt-1]);
        }
        
        stat.m_skip_media = false;
        
        header = &sdp->m_header;
        media = creatMedia();
        codec = &media->m_codec;
        
        tmp.set(stat.m_ip);
        TokenUtil::copy(media->m_ip, sizeof(media->m_ip), &tmp);
        media->m_ttl = stat.m_ttl;
        
        bOk = TokenUtil::next(&tmp, &txt, " ");
        if (!bOk) {
            break;
        }

        if (TokenUtil::strCmp(&tmp, "audio", false)) {
            codec->m_codec_type = AVMEDIA_TYPE_AUDIO;
        } else if (TokenUtil::strCmp(&tmp, "video", false)) {
            codec->m_codec_type = AVMEDIA_TYPE_VIDEO;
        } else if (TokenUtil::strCmp(&tmp, "application", false)) {
            codec->m_codec_type = AVMEDIA_TYPE_DATA;
        } else {
            break;
        }

        /* port */
        bOk = TokenUtil::nextInt(&media->m_port, &txt, " ");
        if (!bOk) {
            break;
        }

        /* protocol rtp/avp */
        bOk = TokenUtil::next(&tmp, &txt, " ");
        if (!bOk) {
            break;
        }

        media->m_is_udp = false;
        for (int i=0; i<DEF_RTP_PROTOCOL_CNT; ++i) {
            bOk = TokenUtil::next(&val, &tmp, "/");
            if (bOk) {
                bOk = TokenUtil::strCmp(&val, 
                    DEF_RTP_PROTOCOL[i], true);
                if (!bOk) {
                    break;
                }
            } else {
                media->m_is_udp = true;
                break;
            }
        }
        
        if (!media->m_is_udp) {
            break;
        }

        /* payload */
        bOk = TokenUtil::nextInt(&codec->m_payload, &txt, " ");
        if (!bOk) {
            break;
        }

        /* set a default url */
        tmp.set(header->m_url);
        TokenUtil::copy(media->m_url, sizeof(media->m_url), &tmp);
        
        media->m_idx = sdp->m_media_cnt;
        sdp->m_media[sdp->m_media_cnt] = media;
        ++sdp->m_media_cnt;
        stat.m_skip_media = false;
        return;
    } while (false);

    freeMedia(media);
    stat.m_skip_media = true;
}

bool SdpUtil::getCodecID(AVCodec* codec, const Token& name) {
    Token tmp;
    
    for (int i=0; rtp_payload_types[i].pt >= 0; i++) {
        if (TokenUtil::strCmp(&name, rtp_payload_types[i].enc_name, true)
            && AV_CODEC_ID_NONE != rtp_payload_types[i].codec_id) { 
            codec->m_codec_id = rtp_payload_types[i].codec_id;

            return true;
        }
    }

    return false;
}

