using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace NZBDrive.Model
{
    public class NewsServerStatusData
    {
        public double ConnectionDataRate { get; set; }
        public DataRateUnit ConnectionDataRateUnit { get; set; }
        public ulong MissingSegmentCount { get; set; }
        public ulong ConnectionTimeoutCount { get; set; }
        public ulong ErrorCount { get; set; }
        public ObservableCollection<NewsServerConnectionStatus> ConnectionStatusCollection { get; set; }
        public int ConnectionCount { get { return ConnectionStatusCollection.Count; } }
    }

    public class NewsServerStatus : NewsServerStatusData, INotifyPropertyChanged
    {
        new public double ConnectionDataRate 
        {
            get { return base.ConnectionDataRate; }
            set { base.ConnectionDataRate = value; OnPropertyChanged(); }  
        }
        new public DataRateUnit ConnectionDataRateUnit 
        {
            get { return base.ConnectionDataRateUnit; }
            set { base.ConnectionDataRateUnit = value; OnPropertyChanged(); } 
        }
        new public ulong MissingSegmentCount
        {
            get { return base.MissingSegmentCount; }
            set { base.MissingSegmentCount = value; OnPropertyChanged(); }
        }
        new public ulong ConnectionTimeoutCount
        {
            get { return base.ConnectionTimeoutCount; }
            set { base.ConnectionTimeoutCount = value; OnPropertyChanged(); }
        }
        new public ulong ErrorCount
        {
            get { return base.ErrorCount; }
            set { base.ErrorCount = value; OnPropertyChanged(); }
        }
        new public ObservableCollection<NewsServerConnectionStatus> ConnectionStatusCollection 
        { 
            get { return base.ConnectionStatusCollection; }
            set { base.ConnectionStatusCollection = value; OnPropertyChanged(); } 
        }
        new public int ConnectionCount { get { return base.ConnectionStatusCollection.Count; } }

        internal void NewsConnectionEvent(NZBDriveDLL.ConnectionState state, int thread)
        {
            NewsServerConnectionStatus status = NewsServerConnectionStatus.Disconnected;

            switch (state)
            {
                case NZBDriveDLL.ConnectionState.Disconnected: status = NewsServerConnectionStatus.Disconnected; break;
                case NZBDriveDLL.ConnectionState.Connecting: status = NewsServerConnectionStatus.Disconnected; break;
                case NZBDriveDLL.ConnectionState.Idle: status = NewsServerConnectionStatus.Idle; break;
                case NZBDriveDLL.ConnectionState.Working: status = NewsServerConnectionStatus.Working; break;
            }

            ConnectionStatusCollection[thread] = status;
        }

        public event PropertyChangedEventHandler PropertyChanged;
        protected virtual void OnPropertyChanged([CallerMemberName] string propertyName = "")
        {
            PropertyChangedEventHandler handler = PropertyChanged;
            if (handler != null) handler(this, new PropertyChangedEventArgs(propertyName));
        }


        ulong bytesRXLast = 0;
        ulong bytesRXCurrent = 0;
        long timeLast = 0;
        long timeCurrent = 0;
        long RC = 10000;
        long rateRX = 0;
        long scale = 1000;

        internal void ServerStateUpdated(long time, ulong bytesTX, ulong bytesRX, uint missingSegmentCount, uint connectionTimeoutCount, uint errorCount)
        {
            MissingSegmentCount = missingSegmentCount;
            ConnectionTimeoutCount = connectionTimeoutCount;
            ErrorCount = errorCount;

            if (time != timeCurrent)
            {

                long dt = timeCurrent - timeLast;
                if (dt > 0)
                {
                    long db = (long)(bytesRXCurrent - bytesRXLast);
                    long alpha = (scale * dt) / (dt + RC);
                    rateRX = alpha * db + (scale - alpha) * db;

                    var rate = 0.01 * (double)rateRX;
                    if ((rate /= 1024) < 512)
                    {
                        ConnectionDataRate = rate;
                        ConnectionDataRateUnit = DataRateUnit.KiBs;
                    }
                    else if ((rate /= 1024) < 512)
                    {
                        ConnectionDataRate = rate;
                        ConnectionDataRateUnit = DataRateUnit.MiBs;
                    }
                    else
                    {
                        rate /= 1024;
                        ConnectionDataRate = rate;
                        ConnectionDataRateUnit = DataRateUnit.GiBs;
                    }
                }

                bytesRXLast = bytesRXCurrent;
                timeLast = timeCurrent;
                timeCurrent = time;
                bytesRXCurrent = 0;
            }


            bytesRXCurrent += bytesRX;
        }
    }

}
