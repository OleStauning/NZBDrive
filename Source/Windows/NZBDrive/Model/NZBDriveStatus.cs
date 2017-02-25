using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace NZBDrive.Model
{
    public class NZBDriveStatusData
    {
        public double ConnectionDataRate { get; set; }
        public DataRateUnit ConnectionDataRateUnit { get; set; }
    }

    public class NZBDriveStatus : NZBDriveStatusData, INotifyPropertyChanged
    {        
        private NZBDriveDLL.NZBDrive nzbDrive;

        public NZBDriveStatus(NZBDriveDLL.NZBDrive nzbDrive)
        {
            // TODO: Complete member initialization
            this.nzbDrive = nzbDrive;
        }

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

        ulong bytesRXLast=0;
        ulong bytesRXCurrent=0;
        long timeLast = 0;
        long timeCurrent = 0;
        long RC = 10000;
        long rateRX = 0;
        long scale = 1000;

        internal void ServerStateUpdated(long time, int server, ulong bytesTX, ulong bytesRX, uint missingSegmentCount, uint connectionTimeoutCount, uint errorCount)
        {
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


        public event PropertyChangedEventHandler PropertyChanged;
        protected virtual void OnPropertyChanged([CallerMemberName] string propertyName = "")
        {
            PropertyChangedEventHandler handler = PropertyChanged;
            if (handler != null) handler(this, new PropertyChangedEventArgs(propertyName));
        }

    }
}
