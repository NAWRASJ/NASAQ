// SPDX-License-Identifier: GPL-3.0-or-later
#include "windowsstartup.h"
#include "packageidentity.h"
#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QUrl>
#include <vector>
#if defined(Q_OS_WIN) && defined(_MSC_VER)
#include <roapi.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Devices.Enumeration.h>
#endif
QString Nasaq::defaultAudioSettingsUri(){
#ifdef Q_OS_WIN
 if(isPackaged()){
  UINT32 length=0;
  if(GetCurrentApplicationUserModelId(&length,nullptr)==ERROR_INSUFFICIENT_BUFFER){
   std::vector<wchar_t> id(length);
   if(GetCurrentApplicationUserModelId(&length,id.data())==ERROR_SUCCESS)
    return QStringLiteral("ms-settings:defaultapps?registeredAUMID=")+QString::fromLatin1(QUrl::toPercentEncoding(QString::fromWCharArray(id.data())));
  }
  return QStringLiteral("ms-settings:defaultapps");
 }
#endif
 return QStringLiteral("ms-settings:defaultapps?registeredAppUser=NASAQ");
}
QString Nasaq::setWindowsStartup(bool enabled) {
#ifdef Q_OS_WIN
 if(isPackaged()) {
#ifdef _MSC_VER
  try {
   winrt::init_apartment(winrt::apartment_type::multi_threaded);
   struct Apartment { ~Apartment(){winrt::uninit_apartment();} } apartment;
   using namespace winrt::Windows::ApplicationModel;
   auto task=StartupTask::GetAsync(L"NASAQStartup").get();
   if(!enabled){
    task.Disable();
    const auto state=task.State();
    if(state==StartupTaskState::Enabled || state==StartupTaskState::EnabledByPolicy)
     return QStringLiteral("Windows policy keeps startup enabled. Contact your administrator.");
    return {};
   }
   const auto state=task.RequestEnableAsync().get();
   if(state==StartupTaskState::Enabled || state==StartupTaskState::EnabledByPolicy)return {};
   return QStringLiteral("Windows did not enable startup. Check Settings > Apps > Startup; it may be disabled by you or your administrator.");
  } catch(const winrt::hresult_error &error) {
   return QString::fromWCharArray(error.message().c_str());
  }
#else
  return QStringLiteral("Packaged startup requires the MSVC build.");
#endif
 }
 QSettings run(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),QSettings::NativeFormat);
 if(enabled)run.setValue(QStringLiteral("NASAQ"),QStringLiteral("\"")+QDir::toNativeSeparators(QCoreApplication::applicationFilePath())+QStringLiteral("\""));
 else run.remove(QStringLiteral("NASAQ"));
 run.sync();
 return run.status()==QSettings::NoError?QString():QStringLiteral("Could not update Windows startup settings.");
#else
 Q_UNUSED(enabled);
 return QStringLiteral("Windows startup is not available on this platform.");
#endif
}
bool Nasaq::microphonePrivacyDenied() {
#if defined(Q_OS_WIN) && defined(_MSC_VER)
 // Observe privacy state only. Never change it or trigger recording here.
 const HRESULT initialized=RoInitialize(RO_INIT_SINGLETHREADED);
 struct Apartment { bool owned; ~Apartment(){if(owned)RoUninitialize();} } apartment{SUCCEEDED(initialized)};
 try {
  using namespace winrt::Windows::Devices::Enumeration;
  const auto access=DeviceAccessInformation::CreateFromDeviceClass(DeviceClass::AudioCapture).CurrentStatus();
  return access==DeviceAccessStatus::DeniedByUser || access==DeviceAccessStatus::DeniedBySystem;
 } catch(const winrt::hresult_error &) {
  // The audio backend still enforces permission if this optional query fails.
 }
#endif
 return false;
}
