/****************************************************************************
** Meta object code from reading C++ file 'VideoStreamPanel.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.15)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../include/MainWindow/VideoStreamPanel.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'VideoStreamPanel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.15. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_VideoStreamPanel_t {
    QByteArrayData data[8];
    char stringdata0[106];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_VideoStreamPanel_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_VideoStreamPanel_t qt_meta_stringdata_VideoStreamPanel = {
    {
QT_MOC_LITERAL(0, 0, 16), // "VideoStreamPanel"
QT_MOC_LITERAL(1, 17, 13), // "streamClicked"
QT_MOC_LITERAL(2, 31, 0), // ""
QT_MOC_LITERAL(3, 32, 5), // "index"
QT_MOC_LITERAL(4, 38, 19), // "streamDoubleClicked"
QT_MOC_LITERAL(5, 58, 19), // "fullscreenRequested"
QT_MOC_LITERAL(6, 78, 21), // "onLayoutButtonClicked"
QT_MOC_LITERAL(7, 100, 5) // "count"

    },
    "VideoStreamPanel\0streamClicked\0\0index\0"
    "streamDoubleClicked\0fullscreenRequested\0"
    "onLayoutButtonClicked\0count"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_VideoStreamPanel[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   34,    2, 0x06 /* Public */,
       4,    1,   37,    2, 0x06 /* Public */,
       5,    1,   40,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       6,    1,   43,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,

 // slots: parameters
    QMetaType::Void, QMetaType::Int,    7,

       0        // eod
};

void VideoStreamPanel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<VideoStreamPanel *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->streamClicked((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->streamDoubleClicked((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->fullscreenRequested((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->onLayoutButtonClicked((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (VideoStreamPanel::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VideoStreamPanel::streamClicked)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (VideoStreamPanel::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VideoStreamPanel::streamDoubleClicked)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (VideoStreamPanel::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VideoStreamPanel::fullscreenRequested)) {
                *result = 2;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject VideoStreamPanel::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_VideoStreamPanel.data,
    qt_meta_data_VideoStreamPanel,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *VideoStreamPanel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *VideoStreamPanel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_VideoStreamPanel.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int VideoStreamPanel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void VideoStreamPanel::streamClicked(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void VideoStreamPanel::streamDoubleClicked(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void VideoStreamPanel::fullscreenRequested(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
