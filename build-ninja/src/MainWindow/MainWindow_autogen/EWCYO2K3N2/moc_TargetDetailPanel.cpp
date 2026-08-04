/****************************************************************************
** Meta object code from reading C++ file 'TargetDetailPanel.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.15)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../include/MainWindow/TargetDetailPanel.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'TargetDetailPanel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.15. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_TargetDetailPanel_t {
    QByteArrayData data[11];
    char stringdata0[167];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_TargetDetailPanel_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_TargetDetailPanel_t qt_meta_stringdata_TargetDetailPanel = {
    {
QT_MOC_LITERAL(0, 0, 17), // "TargetDetailPanel"
QT_MOC_LITERAL(1, 18, 19), // "createTaskRequested"
QT_MOC_LITERAL(2, 38, 0), // ""
QT_MOC_LITERAL(3, 39, 8), // "targetId"
QT_MOC_LITERAL(4, 48, 25), // "markAsFalseAlarmRequested"
QT_MOC_LITERAL(5, 74, 21), // "ignoreTargetRequested"
QT_MOC_LITERAL(6, 96, 12), // "onCreateTask"
QT_MOC_LITERAL(7, 109, 18), // "onMarkAsFalseAlarm"
QT_MOC_LITERAL(8, 128, 14), // "onIgnoreTarget"
QT_MOC_LITERAL(9, 143, 11), // "onPrevImage"
QT_MOC_LITERAL(10, 155, 11) // "onNextImage"

    },
    "TargetDetailPanel\0createTaskRequested\0"
    "\0targetId\0markAsFalseAlarmRequested\0"
    "ignoreTargetRequested\0onCreateTask\0"
    "onMarkAsFalseAlarm\0onIgnoreTarget\0"
    "onPrevImage\0onNextImage"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TargetDetailPanel[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   54,    2, 0x06 /* Public */,
       4,    1,   57,    2, 0x06 /* Public */,
       5,    1,   60,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       6,    0,   63,    2, 0x08 /* Private */,
       7,    0,   64,    2, 0x08 /* Private */,
       8,    0,   65,    2, 0x08 /* Private */,
       9,    0,   66,    2, 0x08 /* Private */,
      10,    0,   67,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void TargetDetailPanel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TargetDetailPanel *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->createTaskRequested((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->markAsFalseAlarmRequested((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->ignoreTargetRequested((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->onCreateTask(); break;
        case 4: _t->onMarkAsFalseAlarm(); break;
        case 5: _t->onIgnoreTarget(); break;
        case 6: _t->onPrevImage(); break;
        case 7: _t->onNextImage(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (TargetDetailPanel::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TargetDetailPanel::createTaskRequested)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (TargetDetailPanel::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TargetDetailPanel::markAsFalseAlarmRequested)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (TargetDetailPanel::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TargetDetailPanel::ignoreTargetRequested)) {
                *result = 2;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject TargetDetailPanel::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_TargetDetailPanel.data,
    qt_meta_data_TargetDetailPanel,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *TargetDetailPanel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TargetDetailPanel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_TargetDetailPanel.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int TargetDetailPanel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void TargetDetailPanel::createTaskRequested(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void TargetDetailPanel::markAsFalseAlarmRequested(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void TargetDetailPanel::ignoreTargetRequested(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
