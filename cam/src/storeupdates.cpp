// SPDX-License-Identifier: GPL-3.0-or-later
#include "storeupdates.h"
#include "nasaqlanguage.h"
#include "packageidentity.h"
#include <QtWidgets>
#include <QtConcurrent/QtConcurrentRun>
#include <QFutureWatcher>
#include <chrono>
#if defined(Q_OS_WIN) && defined(_MSC_VER)
#include <shobjidl.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Services.Store.h>
#endif
StoreUpdateResult queryStoreUpdates(quintptr owner){
#if defined(Q_OS_WIN) && defined(_MSC_VER)
 if(!Nasaq::isPackaged())return {false,QStringLiteral("Microsoft Store installation required.")};
 try{
  winrt::init_apartment(winrt::apartment_type::multi_threaded);
  struct Apartment{~Apartment(){winrt::uninit_apartment();}} apartment;
  auto context=winrt::Windows::Services::Store::StoreContext::GetDefault();
  winrt::check_hresult(context.as<IInitializeWithWindow>()->Initialize(reinterpret_cast<HWND>(owner)));
  auto operation=context.GetAppAndOptionalStorePackageUpdatesAsync();
  if(operation.wait_for(std::chrono::seconds(20))!=winrt::Windows::Foundation::AsyncStatus::Completed){operation.Cancel();return {false,QStringLiteral("Microsoft Store did not complete the update check.")};}
  return {operation.GetResults().Size()>0,{}};
 }catch(const winrt::hresult_error &e){return {false,QString::fromWCharArray(e.message().c_str())};}
#else
 Q_UNUSED(owner);return {false,QStringLiteral("Store update checking requires the Windows MSVC build.")};
#endif
}
StoreUpdates::StoreUpdates(bool packaged,QWidget *parent,Checker checker):QWidget(parent),packaged(packaged),checker(std::move(checker)){
 setObjectName("storeUpdates");auto layout=new QVBoxLayout(this);layout->setContentsMargins(0,0,0,0);
 status=new QLabel;status->setObjectName("storeUpdateStatus");status->setWordWrap(true);layout->addWidget(status);
 auto buttons=new QHBoxLayout;layout->addLayout(buttons);
 checkButton=new QPushButton;checkButton->setObjectName("checkStoreUpdates");buttons->addWidget(checkButton);
 openButton=new QPushButton;openButton->setObjectName("openStoreUpdate");buttons->addWidget(openButton);
 connect(checkButton,&QPushButton::clicked,this,&StoreUpdates::check);
 connect(openButton,&QPushButton::clicked,this,[this]{if(!openStore()){error=QStringLiteral("Could not open Microsoft Store.");updateLanguage();}});
 updateLanguage();
 if(packaged){QTimer::singleShot(30000,this,&StoreUpdates::check);auto timer=new QTimer(this);connect(timer,&QTimer::timeout,this,&StoreUpdates::check);timer->start(6*60*60*1000);}
}
bool StoreUpdates::openStore(){
 if(QDesktopServices::openUrl(QUrl("ms-windows-store://pdp/?ProductId=9PH1T4418KTF")))return true;
 return QDesktopServices::openUrl(QUrl("https://apps.microsoft.com/detail/9PH1T4418KTF"));
}
void StoreUpdates::updateLanguage(){
 const bool ar=NasaqLanguage()=="ar";
 checkButton->setText(ar?QStringLiteral("التحقق من التحديثات"):QStringLiteral("Check for updates"));checkButton->setEnabled(packaged&&!busy);
 openButton->setText(ar?QStringLiteral("تحديث عبر Microsoft Store"):QStringLiteral("Update via Microsoft Store"));
 status->setText(busy?(ar?QStringLiteral("جارٍ التحقق من Microsoft Store…"):QStringLiteral("Checking Microsoft Store…")):
 !error.isEmpty()?(ar?QStringLiteral("تعذر التحقق من التحديثات. أعد المحاولة أو افتح المتجر."):QStringLiteral("Could not check for updates. Retry or open Microsoft Store.")):
 available?(ar?QStringLiteral("يتوفر تحديث جديد. افتح Microsoft Store لتحديث التطبيق عندما يناسبك."):QStringLiteral("An update is available. Open Microsoft Store to update when convenient.")):
 checked?(ar?QStringLiteral("لا يوجد تحديث متاح لهذا الجهاز حاليًا في Microsoft Store."):QStringLiteral("No update is currently available for this device in Microsoft Store.")):
 packaged?(ar?QStringLiteral("يتم التحقق من التحديثات تلقائيًا أثناء فتح التطبيق."):QStringLiteral("Updates are checked automatically while the app is open.")):
 (ar?QStringLiteral("ثبّت نسخة Microsoft Store للاستفادة من فحص تحديثات المتجر تلقائيًا."):QStringLiteral("Install the Microsoft Store version to check Store updates automatically.")));
 status->setToolTip(error);
}
void StoreUpdates::check(){
 if(busy||!packaged)return;
 busy=true;updateLanguage();auto watcher=new QFutureWatcher<StoreUpdateResult>(this);
 connect(watcher,&QFutureWatcher<StoreUpdateResult>::finished,this,[this,watcher]{auto result=watcher->result();watcher->deleteLater();busy=false;error=result.error;
  // A failed check must not retract an update already found.
  if(error.isEmpty()){checked=true;available=result.available;Q_EMIT availabilityChanged(available);}updateLanguage();
 });
 const auto owner=quintptr(window()->winId());const auto run=checker;
 watcher->setFuture(QtConcurrent::run([run,owner]{try{return run(owner);}catch(...){return StoreUpdateResult{false,QStringLiteral("Update check failed.")};}}));
}
