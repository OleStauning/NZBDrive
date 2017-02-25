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
    public class NZBFileSegmentStatusToColorConverter : IValueConverter
    {
        private SolidColorBrush NotLoadedColor = new SolidColorBrush(Colors.White);
        private SolidColorBrush LoadingColor = new SolidColorBrush(Colors.Gray);
        private SolidColorBrush LoadedColor = new SolidColorBrush(Colors.Green);
        private SolidColorBrush WarningColor = new SolidColorBrush(Colors.Yellow);
        private SolidColorBrush ErrorColor = new SolidColorBrush(Colors.Red);

        public object Convert(object value, Type targetType,
            object parameter, CultureInfo culture)
        {
            switch ((int)value)
            {
                case (int)NZBDriveDLL.SegmentState.None: return NotLoadedColor;
                case (int)NZBDriveDLL.SegmentState.Loading: return LoadingColor;
                case (int)NZBDriveDLL.SegmentState.HasData: return LoadedColor;
                case (int)NZBDriveDLL.SegmentState.MissingSegment: return WarningColor;
                case (int)NZBDriveDLL.SegmentState.DownloadFailed: return ErrorColor;
            }
            return ErrorColor;
        }

        public object ConvertBack(object value, Type targetType,
            object parameter, CultureInfo culture)
        {
            return null;
        }
    }
}
