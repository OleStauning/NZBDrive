using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Configuration;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NZBDrive.Model
{
    public sealed class NZBDriveSettingsData : ApplicationSettingsBase
    {
        [UserScopedSetting()]
        [DefaultSettingValue("")]
        public string LicenseKey
        {
            get { return (string)base["LicenseKey"]; }
            set { base["LicenseKey"] = value; }
        }

        [UserScopedSetting()]
        [DefaultSettingValue(null)]
        public NZBDriveOptionsData Options
        {
            get { return (NZBDriveOptionsData)base["Options"]; }
            set { base["Options"] = value; }
        }

        [UserScopedSetting()]
        [DefaultSettingValue(null)]
        public List<NewsServerConnectionData> UsenetServers
        {
            get { return (List<NewsServerConnectionData>)base["UsenetServers"]; }
            set { base["UsenetServers"] = value; }
        }

        [UserScopedSetting()]
        [DefaultSettingValue(null)]
        public NewsServerThrottlingData Throttling
        {
            get { return (NewsServerThrottlingData)base["Throttling"]; }
            set { base["Throttling"] = value; }
        }

        [UserScopedSetting()]
        [DefaultSettingValue("")]
        public string LatestVersion
        {
            get { return (string)base["LatestVersion"]; }
            set { base["LatestVersion"] = value; }
        }

        [UserScopedSetting()]
        [DefaultSettingValue("")]
        public DateTime LatestVersionCheckDate
        {
            get { return (DateTime)base["LatestVersionCheckDate"]; }
            set { base["LatestVersionCheckDate"] = value; }
        }
    }


    public static class NZBDriveSettings
    { 
        // NOTE: For some reason we have to create _new_ objects - not just reference to the model - for the settings to be stored!??!!?
        public static void Save(NZBDriveModel model)
        {
            NZBDriveSettingsData settings = new NZBDriveSettingsData()
            {
                UsenetServers = model.NewsServerCollection.Select(server => new NewsServerConnectionData(server.ServerConnection)).ToList(),
                Throttling = new NewsServerThrottlingData(model.NewsServerThrottling),
                LatestVersion = model.LatestVersion!=null ? model.LatestVersion.ToString() : "",
                LatestVersionCheckDate = model.LatestVersionCheckDate,
                Options = new NZBDriveOptionsData(model.Options)

            };

            settings.Save();
        }

        public static void Load(NZBDriveModel model)
        {
            NZBDriveSettingsData settings = new NZBDriveSettingsData();
            settings.Reload();

            if (settings.Options != null)
            {
                model.Options.SetValues(settings.Options);
            }

            model.StartDrive();

            if (settings.UsenetServers != null)
            {
                foreach (var server in settings.UsenetServers) model.NewsServerCollection.Add(server);
            }
            if (settings.Throttling != null)
            {
                model.NewsServerThrottling.Update(settings.Throttling);
            }

            model.LatestVersion = new Version();
            if (settings.LatestVersion != null)
            {
                try
                {
                    model.LatestVersion = new Version(settings.LatestVersion);
                }
                catch (Exception)
                { }
            }
            if (settings.LatestVersionCheckDate != null)
            {
                model.LatestVersionCheckDate = settings.LatestVersionCheckDate;
            }

        }

    
    }
}
