#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QByteArray>
#include <QFileDialog>
#include <QString>

#include <fex/fex.h>

#include <gme/gme.h>

#include <ao/ao.h>

#include <dumb.h>

extern "C" {
#include <internal/it.h>
}

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



static DUMBFILE_SYSTEM mem_dfs = {
    NULL, // open
    &dumbfile_mem_skip,
    &dumbfile_mem_getc,
    &dumbfile_mem_getnc,
    NULL // close
};



class PlayerThread : public QThread
{
    volatile bool running;

    QByteArray file;

public:
    PlayerThread(QObject * parent, const char * data, int size)
        : QThread( parent ), file( data, size )
    {
    }

    ~PlayerThread()
    {
        stop();
        wait();
    }

    void run()
    {
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
                        dumb_it_set_resampling_quality( itsr, 2 );
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
                                        if ( (unsigned)(sample + 0x800000) >= 0x1000000 ) sample = ( sample >> 31 ) ^ 0x7fff;
                                        else sample >>= 8;
                                        sample_buffer[ i * 2 + 0 ] = sample;
                                        sample = buf[0][ i * 2 + 1 ];
                                        if ( (unsigned)(sample + 0x800000) >= 0x1000000 ) sample = ( sample >> 31 ) ^ 0x7fff;
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

    void stop()
    {
        running = false;
    }
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    thread(0)
{
    fex_init();

    dumb_it_max_to_mix = DUMB_IT_TOTAL_CHANNELS;
    dumb_register_stdfiles();

    ao_initialize();

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

    ao_shutdown();
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
                    thread = new PlayerThread( this, (const char *)data, size );
                    thread->start();

                    break;
                }

                fex_next( archive );
            }

            fex_close( archive );
        }
    }
}
