#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QByteArray>
#include <QFileDialog>
#include <QString>

#include <gme/blargg_common.h>

#ifdef _WIN32
typedef wchar_t blargg_wchar_t;
#else
typedef uint16_t blargg_wchar_t;
#endif

#include <gme/gme.h>

#include <fex/fex.h>

#include <ao/ao.h>

#include <dumb.h>

extern "C" {
#include <internal/it.h>
}

#include <midi_processor.h>

#include <bassmidi.h>

#include <psflib.h>
#include <psf2fs.h>
#include <mkhebios.h>

#include <psx.h>
#include <iop.h>
#include <r3000.h>
#include <bios.h>

#include <sega.h>
#include <satsound.h>
#include <dcsound.h>
#include <yam.h>

#include "../QSoundCore/Core/qsound.h"

#include "gba/GBA.h"

#include "hvl_replay.h"

#define SAMPLE_RATE 44100

typedef struct tdumbfile_mem_status
{
    const unsigned char * ptr;
    unsigned offset, size;
} dumbfile_mem_status;



static int dumbfile_mem_skip(void * f, long n)
{
    dumbfile_mem_status * s = (dumbfile_mem_status *) f;
    s->offset += n;
    if (s->offset > s->size)
    {
        s->offset = s->size;
        return 1;
    }

    return 0;
}



static int dumbfile_mem_getc(void * f)
{
    dumbfile_mem_status * s = (dumbfile_mem_status *) f;
    if (s->offset < s->size)
    {
        return *(s->ptr + s->offset++);
    }
    return -1;
}



static long dumbfile_mem_getnc(char * ptr, long n, void * f)
{
    dumbfile_mem_status * s = (dumbfile_mem_status *) f;
    long max = s->size - s->offset;
    if (max > n) max = n;
    if (max)
    {
        memcpy(ptr, s->ptr + s->offset, max);
        s->offset += max;
    }
    return max;
}



static int dumbfile_mem_seek(void * f, long n)
{
    dumbfile_mem_status * s = (dumbfile_mem_status *) f;
    if ( n < 0 || n > s->size ) return -1;
    s->offset = n;
    return 0;
}



static long dumbfile_mem_get_size(void * f)
{
    dumbfile_mem_status * s = (dumbfile_mem_status *) f;
    return s->size;
}



static DUMBFILE_SYSTEM mem_dfs = {
    NULL, // open
    &dumbfile_mem_skip,
    &dumbfile_mem_getc,
    &dumbfile_mem_getnc,
    NULL, // close
    &dumbfile_mem_seek,
    &dumbfile_mem_get_size
};



static unsigned int font_count = 0;
static HSOUNDFONT * hFonts = NULL;
static HSTREAM hStream = 0;

static void FreeFonts();
static void LoadFonts(const char * name);

void * stdio_fopen( const char * path )
{
    return fopen( path, "rb" );
}

size_t stdio_fread( void *p, size_t size, size_t count, void *f )
{
    return fread( p, size, count, (FILE*) f );
}

int stdio_fseek( void * f, int64_t offset, int whence )
{
    return fseek( (FILE*) f, offset, whence );
}

int stdio_fclose( void * f )
{
    return fclose( (FILE*) f );
}

long stdio_ftell( void * f )
{
    return ftell( (FILE*) f );
}

psf_file_callbacks stdio_callbacks =
{
    "\\/:",
    stdio_fopen,
    stdio_fread,
    stdio_fseek,
    stdio_fclose,
    stdio_ftell
};

struct psf_loader_state
{
    void * emu;
    bool first;
    unsigned refresh;
};

typedef struct {
    uint32_t pc0;
    uint32_t gp0;
    uint32_t t_addr;
    uint32_t t_size;
    uint32_t d_addr;
    uint32_t d_size;
    uint32_t b_addr;
    uint32_t b_size;
    uint32_t s_ptr;
    uint32_t s_size;
    uint32_t sp,fp,gp,ret,base;
} exec_header_t;

typedef struct {
    char key[8];
    uint32_t text;
    uint32_t data;
    exec_header_t exec;
    char title[60];
} psxexe_hdr_t;

inline unsigned get_be16( void const* p )
{
    return  (unsigned) ((unsigned char const*) p) [0] << 8 |
            (unsigned) ((unsigned char const*) p) [1];
}

inline unsigned get_le32( void const* p )
{
    return  (unsigned) ((unsigned char const*) p) [3] << 24 |
            (unsigned) ((unsigned char const*) p) [2] << 16 |
            (unsigned) ((unsigned char const*) p) [1] <<  8 |
            (unsigned) ((unsigned char const*) p) [0];
}

inline unsigned get_be32( void const* p )
{
    return  (unsigned) ((unsigned char const*) p) [0] << 24 |
            (unsigned) ((unsigned char const*) p) [1] << 16 |
            (unsigned) ((unsigned char const*) p) [2] <<  8 |
            (unsigned) ((unsigned char const*) p) [3];
}

inline void set_le32( void* p, unsigned n )
{
    ((unsigned char*) p) [0] = (unsigned char) n;
    ((unsigned char*) p) [1] = (unsigned char) (n >> 8);
    ((unsigned char*) p) [2] = (unsigned char) (n >> 16);
    ((unsigned char*) p) [3] = (unsigned char) (n >> 24);
}

int psf1_loader(void * context, const uint8_t * exe, size_t exe_size,
                                  const uint8_t * reserved, size_t reserved_size)
{
    psf_loader_state * state = ( psf_loader_state * ) context;

    psxexe_hdr_t *psx = (psxexe_hdr_t *) exe;

    if ( exe_size < 0x800 ) return -1;

    uint32_t addr = get_le32( &psx->exec.t_addr );
    uint32_t size = exe_size - 0x800;

    addr &= 0x1fffff;
    if ( ( addr < 0x10000 ) || ( size > 0x1f0000 ) || ( addr + size > 0x200000 ) ) return -1;

    void * pIOP = psx_get_iop_state( state->emu );
    iop_upload_to_ram( pIOP, addr, exe + 0x800, size );

    if ( !state->refresh )
    {
        if (!strncasecmp((const char *) exe + 113, "Japan", 5)) state->refresh = 60;
        else if (!strncasecmp((const char *) exe + 113, "Europe", 6)) state->refresh = 50;
        else if (!strncasecmp((const char *) exe + 113, "North America", 13)) state->refresh = 60;
    }

    if ( state->first )
    {
        void * pR3000 = iop_get_r3000_state( pIOP );
        r3000_setreg(pR3000, R3000_REG_PC, get_le32( &psx->exec.pc0 ) );
        r3000_setreg(pR3000, R3000_REG_GEN+29, get_le32( &psx->exec.s_ptr ) );
        state->first = false;
    }

    return 0;
}

int psf_info(void * context, const char * name, const char * value)
{
    psf_loader_state * state = ( psf_loader_state * ) context;

    if ( !state->refresh && !strcasecmp( name, "_refresh" ) )
    {
        char * moo;
        state->refresh = strtoul( value, &moo, 10 );
    }

    return 0;
}

static int EMU_CALL virtual_readfile(void *context, const char *path, int offset, char *buffer, int length)
{
    return psf2fs_virtual_readfile(context, path, offset, buffer, length);
}

struct sdsf_loader_state
{
    uint8_t * data;
    size_t data_size;
};

int sdsf_loader(void * context, const uint8_t * exe, size_t exe_size,
                                  const uint8_t * reserved, size_t reserved_size)
{
    if ( exe_size < 4 ) return -1;

    struct sdsf_loader_state * state = ( struct sdsf_loader_state * ) context;

    uint8_t * dst = state->data;

    if ( state->data_size < 4 ) {
        state->data = dst = ( uint8_t * ) malloc( exe_size );
        state->data_size = exe_size;
        memcpy( dst, exe, exe_size );
        return 0;
    }

    uint32_t dst_start = get_le32( dst );
    uint32_t src_start = get_le32( exe );
    dst_start &= 0x7fffff;
    src_start &= 0x7fffff;
    uint32_t dst_len = state->data_size - 4;
    uint32_t src_len = exe_size - 4;
    if ( dst_len > 0x800000 ) dst_len = 0x800000;
    if ( src_len > 0x800000 ) src_len = 0x800000;

    if ( src_start < dst_start )
    {
        uint32_t diff = dst_start - src_start;
        state->data_size = dst_len + 4 + diff;
        state->data = dst = ( uint8_t * ) realloc( dst, state->data_size );
        memmove( dst + 4 + diff, dst + 4, dst_len );
        memset( dst + 4, 0, diff );
        dst_len += diff;
        dst_start = src_start;
        set_le32( dst, dst_start );
    }
    if ( ( src_start + src_len ) > ( dst_start + dst_len ) )
    {
        uint32_t diff = ( src_start + src_len ) - ( dst_start + dst_len );
        state->data_size = dst_len + 4 + diff;
        state->data = dst = ( uint8_t * ) realloc( dst, state->data_size );
        memset( dst + 4 + dst_len, 0, diff );
    }

    memcpy( dst + 4 + ( src_start - dst_start ), exe + 4, src_len );

    return 0;
}

struct qsf_loader_state
{
    uint8_t * key;
    uint32_t key_size;

    uint8_t * z80_rom;
    uint32_t z80_size;

    uint8_t * sample_rom;
    uint32_t sample_size;
};

static int upload_section( struct qsf_loader_state * state, const char * section, uint32_t start,
                           const uint8_t * data, uint32_t size )
{
    uint8_t ** array = NULL;
    uint32_t * array_size = NULL;
    uint32_t max_size = 0x7fffffff;

    if ( !strcmp( section, "KEY" ) ) { array = &state->key; array_size = &state->key_size; max_size = 11; }
    else if ( !strcmp( section, "Z80" ) ) { array = &state->z80_rom; array_size = &state->z80_size; }
    else if ( !strcmp( section, "SMP" ) ) { array = &state->sample_rom; array_size = &state->sample_size; }
    else return -1;

    if ( ( start + size ) < start ) return -1;

    uint32_t new_size = start + size;
    uint32_t old_size = *array_size;
    if ( new_size > max_size ) return -1;

    if ( new_size > old_size ) {
        *array = ( uint8_t * ) realloc( *array, new_size );
        *array_size = new_size;
        memset( (*array) + old_size, 0, new_size - old_size );
    }

    memcpy( (*array) + start, data, size );

    return 0;
}

static int qsf_loader(void * context, const uint8_t * exe, size_t exe_size,
                                  const uint8_t * reserved, size_t reserved_size)
{
    struct qsf_loader_state * state = ( struct qsf_loader_state * ) context;

    for (;;) {
        char s[4];
        if ( exe_size < 11 ) break;
        memcpy( s, exe, 3 ); exe += 3; exe_size -= 3;
        s [3] = 0;
        uint32_t dataofs  = get_le32( exe ); exe += 4; exe_size -= 4;
        uint32_t datasize = get_le32( exe ); exe += 4; exe_size -= 4;
        if ( datasize > exe_size )
            return -1;

        if ( upload_section( state, s, dataofs, exe, datasize ) < 0 )
            return -1;

        exe += datasize;
        exe_size -= datasize;
    }

    return 0;
}

struct gsf_loader_state
{
    int entry_set;
    uint32_t entry;
    uint8_t * data;
    size_t data_size;
};

int gsf_loader(void * context, const uint8_t * exe, size_t exe_size,
                                  const uint8_t * reserved, size_t reserved_size)
{
    if ( exe_size < 12 ) return -1;

    struct gsf_loader_state * state = ( struct gsf_loader_state * ) context;

    unsigned char *iptr;
    unsigned isize;
    unsigned char *xptr;
    unsigned xentry = get_le32(exe + 0);
    unsigned xsize = get_le32(exe + 8);
    unsigned xofs = get_le32(exe + 4) & 0x1ffffff;
    if ( xsize < exe_size - 12 ) return -1;
    if (!state->entry_set)
    {
        state->entry = xentry;
        state->entry_set = 1;
    }
    {
        iptr = state->data;
        isize = state->data_size;
        state->data = 0;
        state->data_size = 0;
    }
    if (!iptr)
    {
        unsigned rsize = xofs + xsize;
        {
            rsize -= 1;
            rsize |= rsize >> 1;
            rsize |= rsize >> 2;
            rsize |= rsize >> 4;
            rsize |= rsize >> 8;
            rsize |= rsize >> 16;
            rsize += 1;
        }
        iptr = (unsigned char *) malloc(rsize + 10);
        if (!iptr)
            return -1;
        memset(iptr, 0, rsize + 10);
        isize = rsize;
    }
    else if (isize < xofs + xsize)
    {
        unsigned rsize = xofs + xsize;
        {
            rsize -= 1;
            rsize |= rsize >> 1;
            rsize |= rsize >> 2;
            rsize |= rsize >> 4;
            rsize |= rsize >> 8;
            rsize |= rsize >> 16;
            rsize += 1;
        }
        xptr = (unsigned char *) realloc(iptr, xofs + rsize + 10);
        if (!xptr)
        {
            free(iptr);
            return -1;
        }
        iptr = xptr;
        isize = rsize;
    }
    memcpy(iptr + xofs, exe + 12, xsize);
    {
        state->data = iptr;
        state->data_size = isize;
    }
    return 0;
}

struct gsf_sound_out : public GBASoundOut
{
    ao_device * dev;
    virtual ~gsf_sound_out() { }
    // Receives signed 16-bit stereo audio and a byte count
    virtual void write(const void * samples, unsigned bytes)
    {
        ao_play(dev, (char*)samples, bytes);
    }
};

class PlayerThread : public QThread
{
    volatile bool running;

    QString path;
    QByteArray file;

public:
    PlayerThread(QObject * parent, QString _path, const char * data, int size)
        : QThread( parent ), path( _path ), file( data, size )
    {
    }

    ~PlayerThread()
    {
        stop();
        wait();
    }

    void run()
    {
        QByteArray path_native = path.toLocal8Bit();

        const char * separator = strrchr( path_native.constData(), '/' );
        const char * extension = strrchr( path_native.constData(), '.' );

        if ( extension && extension > separator ) ++extension;
        else extension = "";

        Music_Emu * emu = NULL;

        ao_device * dev;
        ao_sample_format fmt = { 16, SAMPLE_RATE, 2, AO_FMT_NATIVE, 0 };

        signed short sample_buffer[2048 * 2];

        if ( !gme_open_data( file.constData(), file.size(), &emu, SAMPLE_RATE ) )
        {
            dev = ao_open_live( ao_default_driver_id(), &fmt, NULL );

            if ( dev )
            {
                gme_start_track( emu, 0 );

                running = true;
                while (running)
                {
                    gme_play( emu, 2048 * 2, sample_buffer );
                    ao_play( dev, (char *)sample_buffer, 2048 * 2 * sizeof(short) );
                }

                ao_close( dev );
            }

            gme_delete( emu );
        }
        else
        {
            std::vector<uint8_t> data;

            data.assign( file.constData(), file.constData() + file.size() );

            midi_container midifile;
            if ( midi_processor::process_file( data, extension, midifile ) )
            {
                hStream = BASS_MIDI_StreamCreate( 16, BASS_STREAM_DECODE | BASS_MIDI_SINCINTER, SAMPLE_RATE );
                if ( hStream )
                {
#ifdef _WIN32
                    LoadFonts( "E:\\Users\\Public\\Music\\SoundFonts\\Colossus.SF2\\Colossus_SGM_overlay.sflist" );
#else
                    LoadFonts( "/mnt/purgatory/Users/Public/Music/Soundfonts/Colossus.SF2/Colossus_SGM_overlay.sflist" );
#endif

                    BASS_MIDI_StreamEvent( hStream, 0, MIDI_EVENT_SYSTEM, MIDI_SYSTEM_DEFAULT );

                    std::vector<midi_stream_event> stream;
                    system_exclusive_table sysex;

                    midifile.serialize_as_stream( 0, stream, sysex, midi_container::clean_flag_emidi );

                    midifile.scan_for_loops( true, true );

                    unsigned loop_start = midifile.get_timestamp_loop_start( 0, true );
                    unsigned loop_end   = midifile.get_timestamp_loop_end( 0, true );

                    if ( loop_start == ~0u ) loop_start = 0;
                    if ( loop_end == ~0u ) loop_end = (stream.end() - 1)->m_timestamp;

                    unsigned timestamp = 0;

                    auto it = stream.begin(), end = stream.end();

                    dev = ao_open_live( ao_default_driver_id(), &fmt, NULL );

                    if ( dev )
                    {
                        running = true;

                        while ( it < end && running )
                        {
                            unsigned index = it - stream.begin();
                            unsigned target_timestamp = it->m_timestamp;
                            if ( target_timestamp > loop_end ) target_timestamp = loop_end;
                            unsigned delta = target_timestamp - timestamp;
                            timestamp = target_timestamp;
                            if ( delta )
                            {
                                unsigned int sample_data = ( delta * 441 + 5 ) / 10;
                                while ( sample_data && running )
                                {

                                    unsigned int todo = std::min( sample_data, 2048u );
                                    BASS_ChannelGetData( hStream, &sample_buffer, todo * 2 * sizeof(short) );
                                    ao_play( dev, (char*) sample_buffer, todo * 2 * sizeof(short) );
                                    sample_data -= todo;
                                }
                            }
                            if ( it->m_timestamp > loop_end )
                            {
                                it = stream.begin();
                                while ( it->m_timestamp < loop_start ) ++it;
                                timestamp = it->m_timestamp;
                            }
                            if ( !( it->m_event & 0x80000000 ) )
                            {
                                unsigned int count = 2;
                                unsigned char buffer[ 4 ];
                                buffer[ 0 ] = it->m_event & 0xFF;
                                buffer[ 1 ] = ( it->m_event >> 8 ) & 0xFF;
                                if ( ( buffer[ 0 ] & 0xF0 ) != 0xC0 && ( buffer[ 0 ] & 0xF0 ) != 0xD0 )
                                {
                                    buffer[ 2 ] = ( it->m_event >> 16 ) & 0xFF;
                                    count = 3;
                                }
                                BASS_MIDI_StreamEvents( hStream, BASS_MIDI_EVENTS_RAW, buffer, count );
                            }
                            else
                            {
                                unsigned int index = it->m_event & 0x7FFFFFFF;
                                const uint8_t * s_data;
                                std::size_t s_size;
                                std::size_t s_port;
                                sysex.get_entry( index, s_data, s_size, s_port );
                                BASS_MIDI_StreamEvents( hStream, BASS_MIDI_EVENTS_RAW, s_data, s_size );
                            }
                            if ( ++it >= end )
                            {
                                it = stream.begin();
                                while ( it->m_timestamp < loop_start ) ++it;
                                timestamp = it->m_timestamp;
                            }
                        }

                        ao_close( dev );
                    }

                    BASS_StreamFree( hStream );
                    hStream = 0;

                    FreeFonts();
                }
            }
            else
            {
                int psf_version = psf_load( path.toLocal8Bit().constData(), &stdio_callbacks, 0, 0, 0, 0, 0 );

                if ( psf_version > 0 )
                {
                    if ( psf_version == 1 )
                    {
                        void * psx_state = malloc( psx_get_state_size( 1 ) );

                        if ( psx_state )
                        {
                            psx_clear_state( psx_state, 1 );

                            psf_loader_state state;

                            state.emu = psx_state;
                            state.first = true;
                            state.refresh = 0;

                            if ( psf_load( path.toLocal8Bit().constData(), &stdio_callbacks, 1, psf1_loader, &state, psf_info, &state ) > 0 )
                            {

                                dev = ao_open_live( ao_default_driver_id(), &fmt, NULL );
                                if ( dev )
                                {
                                    if ( state.refresh )
                                    {
                                        psx_set_refresh( psx_state, state.refresh );
                                    }

                                    running = true;

                                    while ( running )
                                    {
                                        uint32_t howmany = 2048;

                                        psx_execute( psx_state, 0x7fffffff, sample_buffer, &howmany, 0 );
                                        ao_play( dev, (char *) sample_buffer, howmany * 2 * sizeof(short) );
                                    }

                                    ao_close( dev );
                                }
                            }

                            free( psx_state );
                        }
                    }
                    else if ( psf_version == 2 )
                    {
                        void * psf2fs = psf2fs_create();

                        if ( psf2fs )
                        {
                            psf_loader_state state;

                            state.refresh = 0;

                            if ( psf_load( path.toLocal8Bit().constData(), &stdio_callbacks, 2, psf2fs_load_callback, psf2fs, psf_info, &state ) > 0 )
                            {
                                void * psx_state = malloc( psx_get_state_size( 2 ) );

                                if ( psx_state )
                                {
                                    psx_clear_state( psx_state, 2 );

                                    if ( state.refresh )
                                    {
                                        psx_set_refresh( psx_state, state.refresh );
                                    }

                                    psx_set_readfile( psx_state, virtual_readfile, psf2fs );

                                    fmt.rate = 48000;

                                    dev = ao_open_live( ao_default_driver_id(), &fmt, NULL );
                                    if ( dev )
                                    {
                                        running = true;
                                        while ( running )
                                        {
                                            uint32_t howmany = 2048;

                                            psx_execute( psx_state, 0x7fffffff, sample_buffer, &howmany, 0 );
                                            ao_play( dev, (char *) sample_buffer, howmany * 2 * sizeof(short) );
                                        }

                                        ao_close( dev );
                                    }

                                    free( psx_state );
                                }
                            }

                            psf2fs_delete( psf2fs );
                        }
                    }
                    else if ( psf_version == 0x11 || psf_version == 0x12 )
                    {
                        sdsf_loader_state state;
                        memset( &state, 0, sizeof(state) );

                        if ( psf_load( path.toLocal8Bit().constData(), &stdio_callbacks, psf_version, sdsf_loader, &state, 0, 0 ) > 0 )
                        {
                            void * sega_state = malloc( sega_get_state_size( psf_version - 0x10 ) );

                            if ( sega_state )
                            {
                                sega_clear_state( sega_state, psf_version - 0x10 );

                                sega_enable_dry( sega_state, 1 );
                                sega_enable_dsp( sega_state, 1 );

                                int dynarec = 1;
                                sega_enable_dsp_dynarec( sega_state, dynarec );

                                void * yam = 0;

                                if ( dynarec )
                                {
                                    if ( psf_version == 0x12 )
                                    {
                                        void * dcsound = sega_get_dcsound_state( sega_state );
                                        yam = dcsound_get_yam_state( dcsound );
                                    }
                                    else
                                    {
                                        void * satsound = sega_get_satsound_state( sega_state );
                                        yam = satsound_get_yam_state( satsound );
                                    }
                                    if ( yam ) yam_prepare_dynacode( yam );
                                }

                                uint32_t start  = *(uint32_t*) state.data;
                                uint32_t length = state.data_size;
                                const uint32_t max_length = ( psf_version == 0x12 ) ? 0x800000 : 0x80000;
                                if ( ( start + ( length - 4 ) ) > max_length ) {
                                    length = max_length - start + 4;
                                }
                                sega_upload_program( sega_state, state.data, length );

                                dev = ao_open_live( ao_default_driver_id(), &fmt, NULL );

                                if ( dev )
                                {
                                    running = true;

                                    while ( running )
                                    {
                                        uint32_t howmany = 2048;

                                        sega_execute( sega_state, 0x7fffffff, sample_buffer, &howmany );
                                        ao_play( dev, (char *) sample_buffer, howmany * 2 * sizeof(short) );
                                    }

                                    ao_close( dev );
                                }

                                if ( dynarec )
                                {
                                    yam_unprepare_dynacode( yam );
                                }

                                free( sega_state );
                            }

                            free( state.data );
                        }
                    }
                    else if ( psf_version == 0x22 )
                    {
                        gsf_loader_state state;
                        memset( &state, 0, sizeof(state) );

                        if ( psf_load( path.toLocal8Bit().constData(), &stdio_callbacks, psf_version, gsf_loader, &state, 0, 0 ) > 0 )
                        {
                            gsf_sound_out * output = new gsf_sound_out;
                            GBASystem * system = new GBASystem;

                            system->cpuIsMultiBoot = ((state.entry >> 24) == 2);

                            CPULoadRom( system, state.data, state.data_size );

                            free( state.data );

                            soundInit( system, output );
                            soundReset( system );

                            CPUInit( system );
                            CPUReset( system );

                            dev = ao_open_live( ao_default_driver_id(), &fmt, NULL );

                            if ( dev )
                            {
                                running = true;

                                output->dev = dev;

                                while ( running )
                                {
                                    CPULoop( system, 250000 );
                                }

                                ao_close( dev );
                            }

                            CPUCleanUp( system );
                            soundShutdown( system );

                            delete system;
                            delete output;
                        }
                    }
                    else if ( psf_version == 0x41 )
                    {
                        qsf_loader_state state;
                        memset( &state, 0, sizeof(state) );

                        if ( psf_load( path.toLocal8Bit().constData(), &stdio_callbacks, psf_version, qsf_loader, &state, 0, 0 ) > 0 )
                        {
                            void * qsound_state = malloc( qsound_get_state_size() );
                            if ( qsound_state )
                            {
                                qsound_clear_state( qsound_state );

                                if(state.key_size == 11) {
                                    uint8_t * ptr = state.key;
                                    uint32_t swap_key1 = get_be32( ptr +  0 );
                                    uint32_t swap_key2 = get_be32( ptr +  4 );
                                    uint32_t addr_key  = get_be16( ptr +  8 );
                                    uint8_t  xor_key   =        *( ptr + 10 );
                                    qsound_set_kabuki_key( qsound_state, swap_key1, swap_key2, addr_key, xor_key );
                                } else {
                                    qsound_set_kabuki_key( qsound_state, 0, 0, 0, 0 );
                                }
                                qsound_set_z80_rom( qsound_state, state.z80_rom, state.z80_size );
                                qsound_set_sample_rom( qsound_state, state.sample_rom, state.sample_size );

                                dev = ao_open_live( ao_default_driver_id(), &fmt, NULL );

                                if ( dev )
                                {
                                    running = true;

                                    while ( running )
                                    {
                                        uint32_t howmany = 2048;

                                        qsound_execute( qsound_state, 0x7fffffff, sample_buffer, &howmany );
                                        ao_play( dev, (char *) sample_buffer, howmany * 2 * sizeof(short) );
                                    }

                                    ao_close( dev );
                                }

                                free( qsound_state );
                            }

                            if ( state.key ) free( state.key );
                            if ( state.z80_rom ) free( state.z80_rom );
                            if ( state.sample_rom ) free( state.sample_rom );
                        }
                    }
                }
                else
                {
                    struct hvl_tune * tune = hvl_LoadTune((const uint8_t *)file.constData(), file.size(), SAMPLE_RATE, 2);
                    if (tune)
                    {
                        int8 * buf = new int8[2048 * 2 * 4];
                        int32 * buf32 = (int32*)buf;
                        if (buf)
                        {
                            dev = ao_open_live( ao_default_driver_id(), &fmt, NULL );

                            if ( dev )
                            {
                                running = true;
                                while (running)
                                {
                                    int const samples_per_frame = SAMPLE_RATE / 50;

                                    hvl_DecodeFrame(tune, buf, buf + 4, 8);

                                    for ( long i = 0; i < samples_per_frame; i++ )
                                    {
                                        int sample = buf32[ i * 2 + 0 ];
                                        if ( (unsigned)(sample + 0x800000) & 0xFF000000 ) sample = ( sample >> 31 ) ^ 0x7fff;
                                        else sample >>= 8;
                                        sample_buffer[ i * 2 + 0 ] = sample;
                                        sample = buf32[ i * 2 + 1 ];
                                        if ( (unsigned)(sample + 0x800000) & 0xFF000000 ) sample = ( sample >> 31 ) ^ 0x7fff;
                                        else sample >>= 8;
                                        sample_buffer[ i * 2 + 1 ] = sample;
                                    }
                                    ao_play( dev, (char *)sample_buffer, samples_per_frame * 2 * sizeof(short) );
                                }

                                ao_close( dev );
                                }

                            delete [] buf;
                        }

                        hvl_FreeTune(tune);
                    }
                    else
                    {
                        dumbfile_mem_status memdata;
                        DUMBFILE * f;

                        memdata.ptr = (const unsigned char *) file.constData();
                        memdata.offset = 0;
                        memdata.size = file.size();

                        f = dumbfile_open_ex(&memdata, &mem_dfs);
                        if ( f )
                        {
                            DUH * duh = dumb_read_any_quick( f, 0, 0 );
                            if ( duh )
                            {
                                DUH_SIGRENDERER * sr = duh_start_sigrenderer( duh, 0, 2, 0 );
                                if ( sr )
                                {
                                    DUMB_IT_SIGRENDERER * itsr = duh_get_it_sigrenderer( sr );
                                    dumb_it_set_resampling_quality( itsr, 3 );
                                    dumb_it_set_ramp_style( itsr, 2 );

                                    sample_t ** buf = allocate_sample_buffer( 2, 2048 );
                                    if ( buf )
                                    {
                                        dev = ao_open_live( ao_default_driver_id(), &fmt, NULL );

                                        if ( dev )
                                        {
                                            const float delta = 65536.0f / SAMPLE_RATE;

                                            running = true;
                                            while (running)
                                            {
                                                dumb_silence( buf[0], 2048 * 2 );
                                                long written = duh_sigrenderer_generate_samples( sr, 1.0f, delta, 2048, buf );
                                                for ( long i = 0; i < written; i++ )
                                                {
                                                    int sample = buf[0][ i * 2 + 0 ];
                                                    if ( (unsigned)(sample + 0x800000) & 0xFF000000 ) sample = ( sample >> 31 ) ^ 0x7fff;
                                                    else sample >>= 8;
                                                    sample_buffer[ i * 2 + 0 ] = sample;
                                                    sample = buf[0][ i * 2 + 1 ];
                                                    if ( (unsigned)(sample + 0x800000) & 0xFF000000 ) sample = ( sample >> 31 ) ^ 0x7fff;
                                                    else sample >>= 8;
                                                    sample_buffer[ i * 2 + 1 ] = sample;
                                                }
                                                ao_play( dev, (char *)sample_buffer, written * 2 * sizeof(short) );
                                                if ( written < 2048 ) break;
                                            }

                                            ao_close( dev );
                                        }

                                        destroy_sample_buffer( buf );
                                    }

                                    duh_end_sigrenderer( sr );
                                }

                                unload_duh( duh );
                            }

                            dumbfile_close( f );
                        }
                    }
                }
            }
        }
    }

    void stop()
    {
        running = false;
    }
};

void FreeFonts()
{
    unsigned i;
    if ( hFonts && font_count )
    {
        BASS_MIDI_StreamSetFonts( hStream, (const BASS_MIDI_FONTEX*) NULL, 0 );
        for ( i = 0; i < font_count; ++i )
        {
            BASS_MIDI_FontFree( hFonts[ i ] );
        }
        free( hFonts );
        hFonts = NULL;
        font_count = 0;
    }
}

void LoadFonts(const char * name)
{
    FreeFonts();

    if (name && *name)
    {
        const char * ext = strrchr( name, '.' );
        if ( ext ) ext++;
        if ( !strcasecmp( ext, "sf2" ) || !strcasecmp( ext, "sf2pack" ) )
        {
            font_count = 1;
            hFonts = (HSOUNDFONT*)malloc( sizeof(HSOUNDFONT) );
            *hFonts = BASS_MIDI_FontInit( name, 0 );
        }
        else if ( !strcasecmp( ext, "sflist" ) )
        {
            FILE * fl = fopen( name, "r" );
            font_count = 0;
            if ( fl )
            {
                char path[PATH_MAX], fontname[PATH_MAX], temp[PATH_MAX];
                const char * filename = strrchr( name, '/' ) + 1;
                if ( filename == (char*)1 ) filename = name;
                strncpy( path, name, filename - name );
                path[ filename - name ] = 0;
                while ( !feof( fl ) )
                {
                    char * cr, * slash;
                    if( !fgets( fontname, PATH_MAX, fl ) ) break;
                    fontname[PATH_MAX-1] = 0;
                    cr = strrchr( fontname, '\n' );
                    if ( cr ) *cr = 0;
                    cr = strrchr( fontname, '\r' );
                    if ( cr ) *cr = 0;
                    if ( fontname[0] == '/' )
                    {
                        cr = fontname;
                    }
                    else
                    {
                        size_t i = strlen(path) + strlen(fontname);
                        strncpy( temp, path, PATH_MAX );
                        strncat( temp, fontname, i < PATH_MAX ? i : PATH_MAX );
                        temp[PATH_MAX - 1] = 0;
                        cr = temp;
                    }
                    while (slash = strchr(cr, '\\')) *slash = '/';
                    font_count++;
                    hFonts = (HSOUNDFONT*)realloc( hFonts, sizeof(HSOUNDFONT) * font_count );
                    hFonts[ font_count - 1 ] = BASS_MIDI_FontInit( cr, 0 );
                }
                fclose( fl );
            }
        }
        if (font_count) {
            int i;
            BASS_MIDI_FONT * mf = (BASS_MIDI_FONT*)malloc( sizeof(BASS_MIDI_FONT) * font_count );
            for ( i = 0; i < font_count; ++i ) {
                mf[i].font = hFonts[ font_count - i - 1 ];
                mf[i].preset = -1;
                mf[i].bank = 0;
            }
            BASS_MIDI_StreamSetFonts( hStream, mf, font_count );
            free(mf);
        }
    }
}



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    thread(0)
{
    fex_init();

    dumb_it_max_to_mix = DUMB_IT_TOTAL_CHANNELS;
    dumb_register_stdfiles();

    ao_initialize();

    BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 10);

    BASS_Init( 0, SAMPLE_RATE, 0, NULL, NULL );

#if defined(_WIN32) || defined(__APPLE__)
    BASS_PluginLoad( "bassflac.dll", 0 );
    BASS_PluginLoad( "basswv.dll", 0 );
#else
    BASS_PluginLoad( "/home/chris/src/bass/x64/libbassflac.so", 0 );
    BASS_PluginLoad( "/home/chris/src/bass/x64/libbasswv.so", 0 );
#endif

    {
#ifdef _WIN32
        FILE * f = fopen( "E:\\emulate\\ps2\\pcsx2\\bios\\Scph39001.bin", "rb" );
#elif defined(__APPLE__)
        FILE * f = fopen( "/Volumes/Purgatory/emulate/ps2/pcsx2/bios/Scph39001.bin", "rb");
#else
        FILE * f = fopen( "/home/chris/ps2bios", "rb" );
#endif
        fseek( f, 0, SEEK_END );
        int bios_size = ftell( f );
        fseek( f, 0, SEEK_SET );

        uint8_t * bios = ( uint8_t * ) malloc( bios_size );
        fread( bios, 1, bios_size, f );
        fclose( f );

        void * bios_processed = mkhebios_create( bios, &bios_size );

        /*f = fopen( "/home/chris/hebios", "wb" );
        fwrite( bios_processed, 1, bios_size, f );
        fclose( f );*/

        bios_set_image( (uint8 *) bios_processed, bios_size );
    }

    psx_init();

    sega_init();

    qsound_init();

    hvl_InitReplayer();

    ui->setupUi(this);

    connect(ui->actionOpen,
            SIGNAL(triggered()),
            this,
            SLOT(appFileOpen()));

    connect(ui->actionExit,
            SIGNAL(triggered()),
            qApp,
            SLOT(quit()));
}

MainWindow::~MainWindow()
{
    delete thread;
    delete ui;

    BASS_Free();

    ao_shutdown();

    mkhebios_delete( bios_get_image_native() );
}

static const char * exts_to_block[] = { "txt", "nfo", "info", "diz" };

void MainWindow::appFileOpen()
{
    QString path = QFileDialog::getOpenFileName( this, "Select a song..." );

    if ( path.length() )
    {
        delete thread;

        fex_t * archive;

        if ( 0 == fex_open( &archive, path.toLocal8Bit().constData() ) )
        {
            while ( !fex_done( archive ) )
            {
                const char * name = fex_name( archive );
                fex_stat( archive );
                uint64_t size = fex_size( archive );

                if ( size )
                {
                    const char * dot = strrchr( name, '.' );
                    if ( dot )
                    {
                        for ( unsigned i = 0; i < 4; i++ )
                        {
                            if ( !strcasecmp( dot + 1, exts_to_block[ i ] ) )
                            {
                                fex_next( archive );
                                continue;
                            }
                        }
                    }

                    const void * data;
                    fex_data( archive, &data );
                    thread = new PlayerThread( this, QString::fromLocal8Bit( name ), (const char *)data, size );
                    thread->start();

                    break;
                }

                fex_next( archive );
            }

            fex_close( archive );
        }
    }
}
