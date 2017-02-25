using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Data;
using System.Windows.Media;
using NZBDrive.Model;

namespace NZBDrive.View
{
    public class ConnectionStatusToColorConverter : IValueConverter
    {
        private SolidColorBrush DisconnectedColor = new SolidColorBrush(Colors.White);
        private SolidColorBrush ConnectingColor = new SolidColorBrush(Colors.Gray);
        private SolidColorBrush IdleColor = new SolidColorBrush(Colors.LightGreen);
        private SolidColorBrush WorkingColor = new SolidColorBrush(Colors.Green);

        public object Convert(object value, Type targetType,
            object parameter, CultureInfo culture)
        {
            switch ((int)value)
            {
                case (int)NewsServerConnectionStatus.Disconnected: return DisconnectedColor;
                case (int)NewsServerConnectionStatus.Connecting: return ConnectingColor;
                case (int)NewsServerConnectionStatus.Idle: return IdleColor;
                case (int)NewsServerConnectionStatus.Working: return WorkingColor;
            }
            return DisconnectedColor;
        }

        public object ConvertBack(object value, Type targetType,
            object parameter, CultureInfo culture)
        {
            return null;
        }
    }
}
