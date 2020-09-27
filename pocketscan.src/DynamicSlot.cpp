
/*
 * Copyright (c) 2010    Aleksander B. Demko
 * This source code is distributed under the MIT license.
 * See the accompanying file LICENSE.MIT.txt for details.
 */

#include <DynamicSlot.h>

DynamicSlot::Handler::~Handler() {}

void DynamicSlot::trigger(void) {
    dm_sender = QObject::sender();
    dm_h->trigger();
    dm_sender = 0;
}

void DynamicSlot::init(void) { dm_sender = 0; }
