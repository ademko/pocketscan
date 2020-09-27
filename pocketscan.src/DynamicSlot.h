
/*
 * Copyright (c) 2010    Aleksander B. Demko
 * This source code is distributed under the MIT license.
 * See the accompanying file LICENSE.MIT.txt for details.
 */

#ifndef __INCLUDED_DYNAMICSLOT_H__
#define __INCLUDED_DYNAMICSLOT_H__

#include <QObject>

#include <memory>

class DynamicSlot;

/**
 * This is a utility class that can accept signals (ie. it has a slot).
 * it then bounces those signals to another normal (non-slot) method
 * and (in the future) normal functions.
 *
 * This adds some flexibility to Qt's signal-slot mechanism that
 * isn't there out of the box.
 *
 * @author Aleksander Demko
 */
class DynamicSlot : public QObject {
    Q_OBJECT

  private:
    class Handler {
      public:
        virtual ~Handler();

        virtual void trigger(void) = 0;
    };

    template <class C> class MethodHander : public Handler {
      public:
        MethodHander(C *objinstance, void (C::*method)(void))
            : dm_objinstance(objinstance), dm_method(method) {}
        virtual void trigger(void) { (dm_objinstance->*dm_method)(); }

      private:
        C *dm_objinstance;
        void (C::*dm_method)(void);
    };

  public:
    // typedef void (C1::*handler)(handler_event&);

    /**
     * Constructor for making a dynamic slot for class method
     * functions.
     *
     * @author Aleksander Demko
     */
    template <class C>
    DynamicSlot(C *objinstance, void (C::*method)(void))
        : dm_h(new MethodHander<C>(objinstance, method)) {
        init();
    }

    // future, add static function-ctor

    /**
     * Returns the sender object that triggered the signal.
     * This will only be non-zero while this is currently
     * reflecting a signal.
     *
     * This is the same as QObject::sender().
     *
     * No, this is not thread-safe. Maybe move this to TLS
     * in the future.
     *
     * @author Aleksander Demko
     */
    QObject *sender(void) const { return dm_sender; }

  public slots:
    /**
     * This is the public slot that can be connected
     * to triggered.
     *
     * @author Aleksander Demko
     */
    void trigger(void);

  private:
    void init(void);

  private:
    std::auto_ptr<Handler> dm_h;
    QObject *dm_sender;
};

#endif
