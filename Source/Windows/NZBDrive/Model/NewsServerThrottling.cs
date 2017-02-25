using System.ComponentModel;

namespace NZBDrive.Model
{
    public class NewsServerThrottlingData
    {
        public NewsServerThrottlingData()
        {
            ThrottlingMode = Model.ThrottlingMode.Adaptive;
            SpeedLimit = 10;
            SpeedLimitUnit = DataRateUnit.Unlimited;
            PreCacheSize = 10;
            PreCacheSizeUnit = DataSizeUnit.MiB;
        }
        public NewsServerThrottlingData(NewsServerThrottlingData other)
        {
            SetValues(other);
        }
        internal void SetValues(NewsServerThrottlingData other)
        {
            this.ThrottlingMode = other.ThrottlingMode;
            this.SpeedLimit = other.SpeedLimit;
            this.SpeedLimitUnit = other.SpeedLimitUnit;
            this.PreCacheSize = other.PreCacheSize;
            this.PreCacheSizeUnit = other.PreCacheSizeUnit;
        }
        public ThrottlingMode ThrottlingMode { get; set; }
        public int SpeedLimit { get; set; }
        public DataRateUnit SpeedLimitUnit { get; set; }
        public int PreCacheSize { get; set; }
        public DataSizeUnit PreCacheSizeUnit { get; set; }
    }

    public class NewsServerThrottling : NewsServerThrottlingData, INotifyPropertyChanged
    {
        private NZBDriveDLL.NZBDrive _nzbDrive;

        public NewsServerThrottling(NZBDriveDLL.NZBDrive nzbDrive)
            :base()
        {
            _nzbDrive = nzbDrive;
        }
        public NewsServerThrottling(NZBDriveDLL.NZBDrive nzbDrive, NewsServerThrottlingData other)
            :base(other)
        {
            _nzbDrive = nzbDrive;
        }
    

        public event PropertyChangedEventHandler PropertyChanged;
        private void RaisePropertyChanged(string propertyName)
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this,
                    new PropertyChangedEventArgs(propertyName));
            }
        }

        public event SettingsChangedDelegate SettingsChanged;
        private void FireChangedEvent()
        {
            var handler = SettingsChanged;
            if (handler != null) handler(this);
        }


        public int ThrottlingModeIndex 
        { 
            get 
            { 
                return (int)ThrottlingMode; 
            } 
            set 
            { 
                ThrottlingMode = (ThrottlingMode)value;
                DoThrottle();
                RaisePropertyChanged("PreCacheEnabled");
                FireChangedEvent();
            } 
        }
        public bool SpeedLimitEnabled { get { return SpeedLimitUnit != DataRateUnit.Unlimited; } }
        public bool PreCacheEnabled { get { return ThrottlingMode == ThrottlingMode.Adaptive; } }

        public int SpeedLimitUnitIndex
        {
            get 
            { 
                return (int)SpeedLimitUnit; 
            }
            set
            {
                SpeedLimitUnit = (DataRateUnit)value;
                RaisePropertyChanged("SpeedLimitEnabled");
                DoThrottle();
                FireChangedEvent();
            }
        }

        public int PreCacheSizeUnitIndex 
        { 
            get 
            { 
                return (int)PreCacheSizeUnit; 
            } 
            set 
            { 
                PreCacheSizeUnit = (DataSizeUnit)value; 
                DoThrottle();
                FireChangedEvent();
            } 
        }

        new public int SpeedLimit 
        { 
            get 
            { 
                return base.SpeedLimit; 
            } 
            set 
            { 
                base.SpeedLimit = value; 
                DoThrottle();
                FireChangedEvent();
            } 
        }

        new public int PreCacheSize 
        { 
            get 
            { 
                return base.PreCacheSize; 
            } 
            set 
            { 
                base.PreCacheSize = value; 
                DoThrottle(); 
            } 
        }


        internal NZBDriveDLL.NetworkMode Adapt(ThrottlingMode mode)
        {
            switch (mode)
            {
                case Model.ThrottlingMode.Adaptive: return NZBDriveDLL.NetworkMode.Adaptive;
                case Model.ThrottlingMode.Constant: return NZBDriveDLL.NetworkMode.Constant;
            }
            return NZBDriveDLL.NetworkMode.Constant;
        }

        internal int Adapt(int value, DataRateUnit unit)
        {
            switch (unit)
            {
                case DataRateUnit.KiBs: return value * 1024;
                case DataRateUnit.MiBs: return value * 1024 * 1024;
                case DataRateUnit.GiBs: return value * 1024 * 1024 * 1024;
                case DataRateUnit.Unlimited: return 0;
            }
            return 0;
        }
        internal int Adapt(int value, DataSizeUnit unit)
        {
            switch (unit)
            {
                case DataSizeUnit.KiB: return value * 1024;
                case DataSizeUnit.MiB: return value * 1024 * 1024;
                case DataSizeUnit.GiB: return value * 1024 * 1024 * 1024;
            }
            return 0;
        }
        
        internal void DoThrottle()
        { 
            _nzbDrive.Throttling = new NZBDriveDLL.NetworkThrottling()
            {
                Mode = Adapt(ThrottlingMode),
                NetworkLimit = Adapt(SpeedLimit,SpeedLimitUnit),
                FastPreCache = Adapt(PreCacheSize,PreCacheSizeUnit),
                AdaptiveIORatioPCT = 110,
                BackgroundNetworkRate = 1000
            };
        }


        internal void Update(NewsServerThrottlingData newsServerThrottlingData)
        {
            if (newsServerThrottlingData != null)
            {
                SetValues(newsServerThrottlingData);
                DoThrottle();
            }
        }
    }
}
