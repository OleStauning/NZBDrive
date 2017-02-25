using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Data;
using System.Windows.Media;

namespace NZBDrive.View
{
    public class LogColorConverter : IValueConverter
    {
        private SolidColorBrush DebugColor = new SolidColorBrush(Colors.White);
        private SolidColorBrush InfoColor = new SolidColorBrush(Colors.White);
        private SolidColorBrush WarningColor = new SolidColorBrush(Colors.Yellow);
        private SolidColorBrush ErrorColor = new SolidColorBrush(Colors.Red);
        private SolidColorBrush FatalColor = new SolidColorBrush(Colors.Red);

        public object Convert(object value, Type targetType,
            object parameter, CultureInfo culture)
        {
            switch ((int)value)
            {
                case (int)NZBDriveDLL.LogLevelType.Debug: return DebugColor;
                case (int)NZBDriveDLL.LogLevelType.Info: return InfoColor;
                case (int)NZBDriveDLL.LogLevelType.PopupInfo: return InfoColor;
                case (int)NZBDriveDLL.LogLevelType.Warning: return WarningColor;
                case (int)NZBDriveDLL.LogLevelType.Error: return ErrorColor;
                case (int)NZBDriveDLL.LogLevelType.PopupError: return ErrorColor;
                case (int)NZBDriveDLL.LogLevelType.Fatal: return FatalColor;
            }
            return InfoColor;
        }

        public object ConvertBack(object value, Type targetType,
            object parameter, CultureInfo culture)
        {
            return null;
        }
    }
}
