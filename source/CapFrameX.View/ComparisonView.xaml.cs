using CapFrameX.ViewModel;
using Microsoft.Win32;
using System;
using System.IO;
using System.Reactive.Linq;
using System.Text.RegularExpressions;
using System.Threading;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace CapFrameX.View
{
	/// <summary>
	/// Interaction logic for ComparisonDataView.xaml
	/// </summary>
	public partial class ComparisonView : UserControl
	{
		public ComparisonView()
		{			
			InitializeComponent();

			var context = SynchronizationContext.Current;
			(DataContext as ComparisonViewModel)?.ResetLShapeChart
				.ObserveOn(context)
				.SubscribeOn(context)
				.Subscribe(x => ResetLShapeChart());
				
			// L-shape chart y axis formatter
			Func<double, string> formatFunc = (x) => string.Format("{0:0.0}", x);
			LShapeY.LabelFormatter = formatFunc;
		}

		private void ResetLShapeChart_MouseDoubleClick(object sender, MouseButtonEventArgs e)
			=> ResetLShapeChart();

		private void ResetLShapeChart()
		{
			//Use the axis MinValue/MaxValue properties to specify the values to display.
			//use double.Nan to clear it.

			LShapeX.MinValue = double.NaN;
			LShapeX.MaxValue = double.NaN;
			LShapeY.MinValue = double.NaN;
			LShapeY.MaxValue = double.NaN;
		}

		private void FirstSecondsTextBox_PreviewKeyDown(object sender, KeyEventArgs e)
		{
			var key = e.Key;

			if (key == Key.Enter)
			{
				GraphTab.Focus();
			}
			(DataContext as ComparisonViewModel).OnRangeSliderValuesChanged();
		}
		
		private void LastSecondsTextBox_PreviewKeyDown(object sender, KeyEventArgs e)
		{
			var key = e.Key;

			if (key == Key.Enter)
			{
				GraphTab.Focus();
			}
			(DataContext as ComparisonViewModel).OnRangeSliderValuesChanged();
		}

		private void CustomTitle_PreviewKeyDown(object sender, KeyEventArgs e)
		{
			var key = e.Key;

			if (key == Key.Enter)
			{
				GraphTab.Focus();
			}
		}

		private void NumberValidationTextBox(object sender, TextCompositionEventArgs e)
		{
			Regex regex = new Regex("[^0-9.-]+");
			e.Handled = regex.IsMatch(e.Text);
		}

        private BitmapSource CaptureGridScreenshot(Grid grid)
        {
            RenderTargetBitmap rtb = new RenderTargetBitmap((int)grid.ActualWidth, (int)grid.ActualHeight-18, 96, 96, PixelFormats.Pbgra32);
            DrawingVisual dv = new DrawingVisual();
            using (DrawingContext dc = dv.RenderOpen())
            {
                VisualBrush vb = new VisualBrush(grid);
                dc.DrawRectangle(vb, null, new Rect(new Point(), new Size(grid.ActualWidth, grid.ActualHeight)));
            }
            rtb.Render(dv);
            return rtb;
        }

        private void SaveGridScreenshot(BitmapSource gridScreenshot)
        {
            SaveFileDialog saveFileDialog = new SaveFileDialog
            {
                Filter = "PNG Image (*.png)|*.png",
                FileName = GameNameTextBlock.Text,
                DefaultExt = ".png",
                AddExtension = true,
                Title = "Save Grid Screenshot"
            };

            if (saveFileDialog.ShowDialog() == true)
            {
                using (FileStream fileStream = new FileStream(saveFileDialog.FileName, FileMode.Create))
                {
                    PngBitmapEncoder encoder = new PngBitmapEncoder();
                    encoder.Frames.Add(BitmapFrame.Create(gridScreenshot));
                    encoder.Save(fileStream);
                }
            }
        }


        private void TakeBarGraphPicture_Click(object sender, RoutedEventArgs e)
        {
			var capture = CaptureGridScreenshot(BarGraphsContainer);

			SaveGridScreenshot(capture);
        }
    }
}
