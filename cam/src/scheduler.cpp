// SPDX-License-Identifier: GPL-3.0-or-later
#include "scheduler.h"
void Scheduler::tick(const QDateTime&now,const std::function<bool(const Event&)>&dispatch){
 for(const auto&e:store.due(now))if(store.claim(e.id)){
  try{bool accepted=dispatch(e);store.finish(e.id,accepted?"dispatched":"rejected");store.log(accepted?"dispatch":"reject",e.id+":"+e.kind);}
  catch(const std::exception&ex){store.finish(e.id,"failed");store.log("dispatch_error",QString::fromUtf8(ex.what()));}
 }
}
