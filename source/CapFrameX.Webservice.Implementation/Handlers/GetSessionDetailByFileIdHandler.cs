﻿using CapFrameX.Webservice.Data.Interfaces;
using CapFrameX.Webservice.Data.Queries;
using MediatR;
using System;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using CapFrameX.Webservice.Data.Extensions;
using Newtonsoft.Json;
using CapFrameX.Data.Session.Classes;
using CapFrameX.Sensor.Reporting;
using CapFrameX.Webservice.Data.DTO;
using CapFrameX.Statistics.PlotBuilder;
using CapFrameX.Statistics.NetStandard.Contracts;
using CapFrameX.Statistics.NetStandard;
using CapFrameX.Statistics.PlotBuilder.Contracts;
using System.IO;
using OxyPlot;
using CapFrameX.Extensions.NetStandard;
using System.ComponentModel;
using CapFrameX.Data.Session.Contracts;
using CapFrameX.Data.Session.Converters;

namespace CapFrameX.Webservice.Implementation.Handlers
{
	public class GetSessionDetailByFileIdHandler : IRequestHandler<GetSessionDetailByFileIdQuery, SessionDetailDTO>
	{
		private readonly ISessionService _sessionService;

		public GetSessionDetailByFileIdHandler(ISessionService sessionService) {
			_sessionService = sessionService;
		}

		public async Task<SessionDetailDTO> Handle(GetSessionDetailByFileIdQuery request, CancellationToken cancellationToken)
		{
			var (fileName, fileBytes) = await _sessionService.DownloadAsset(request.FileId);
			if (fileName.EndsWith(".gz"))
			{
				fileBytes = fileBytes.Decompress();
			}
			var session = JsonConvert.DeserializeObject<Session>(Encoding.UTF8.GetString(fileBytes));
			foreach (var sessionrun in session.Runs)
			{
				if (sessionrun.SensorData != null & sessionrun.SensorData2 == null)
				{
					SessionSensorDataConverter.ConvertToSensorData2(sessionrun);
				}
			}

			var infos = session.Info.GetType().GetProperties().Where(p => p.GetCustomAttributes(false).Any(a => a is DescriptionAttribute)).ToDictionary(p => (p.GetCustomAttributes(false).First(a => a is DescriptionAttribute) as DescriptionAttribute)?.Description, p => Convert.ToString(p.GetValue(session.Info)));
			var sensorItems = SensorReport.GetReportFromSessionSensorData(session.Runs.Select(r => r.SensorData2)).ToArray();

			var frametimeStatisticsProviderOptions = new FrametimeStatisticProviderOptions()
			{
				MovingAverageWindowSize = 500,
				IntervalAverageWindowTime = 500,
				FpsValuesRoundingDigits = 2
			};
			var plotSettings = new PlotSettings();
			var frametimeStatisticProvider = new FrametimeStatisticProvider(frametimeStatisticsProviderOptions);
			var fpsGraphBuilder = new FpsGraphPlotBuilder(frametimeStatisticsProviderOptions, frametimeStatisticProvider);
			fpsGraphBuilder.BuildPlotmodel(session, plotSettings, 0, 1000, ERemoveOutlierMethod.None, EFilterMode.RawPlusAverage);
			var frametimeGraphBuilder = new FrametimePlotBuilder(frametimeStatisticsProviderOptions, frametimeStatisticProvider);
			frametimeGraphBuilder.BuildPlotmodel(session, plotSettings, 0, 1000, ERemoveOutlierMethod.None);

			var exporter = new SvgExporter { Width = 1000, Height = 400 };

			using var frametimeGraphStream = new MemoryStream();
			exporter.Export(frametimeGraphBuilder.PlotModel, frametimeGraphStream);

			using var fpsGraphStream = new MemoryStream();
			exporter.Export(fpsGraphBuilder.PlotModel, fpsGraphStream);

			var frametimes = session.GetFrametimeTimeWindow(0, 1000, frametimeStatisticsProviderOptions, ERemoveOutlierMethod.None);

			var fpsTresholdsLabels = FrametimeStatisticProvider.FPSTHRESHOLDS.ToArray();

			var fpsTresholdsCounts = frametimeStatisticProvider.GetFpsThresholdCounts(frametimes, false).Select(val => (double)val / frametimes.Count()).ToArray();
			var fpsTresholdsCountsDictionary = Enumerable.Range(0, fpsTresholdsLabels.Count()).ToDictionary(idx => (int)fpsTresholdsLabels[idx], idx => fpsTresholdsCounts[idx]).Where(kvp => !double.IsNaN(kvp.Value));

			var fpsTresholdsTimes = frametimeStatisticProvider.GetFpsThresholdTimes(frametimes, false).Select(val => val / frametimes.Sum()).ToArray();
			var fpsTresholdsTimesDictionary = Enumerable.Range(0, fpsTresholdsLabels.Count()).ToDictionary(idx => (int)fpsTresholdsLabels[idx], idx => fpsTresholdsTimes[idx]).Where(kvp => !double.IsNaN(kvp.Value));

			var fpsMetricDictionary = Enum.GetValues(typeof(EMetric)).Cast<EMetric>().ToDictionary(metric => metric.GetAttribute<DescriptionAttribute>().Description, metric => frametimeStatisticProvider.GetFpsMetricValue(frametimes, metric)).Where(kvp => !double.IsNaN(kvp.Value));

			return new SessionDetailDTO()
			{
				Infos = infos,
				SensorItems = sensorItems,
				FpsTresholdsCounts = fpsTresholdsCountsDictionary,
				FpsTresholdsTimes = fpsTresholdsTimesDictionary,
				FpsMetric = fpsMetricDictionary,
				FpsGraph = Encoding.UTF8.GetString(fpsGraphStream.ToArray()),
				FrametimeGraph = Encoding.UTF8.GetString(frametimeGraphStream.ToArray())
			};
		}
	}

	class FrametimeStatisticProviderOptions : IFrametimeStatisticProviderOptions
	{
		public int MovingAverageWindowSize { get; set; }
		public int FpsValuesRoundingDigits { get; set; }
        public int IntervalAverageWindowTime { get; set; }
    }

	class PlotSettings : IPlotSettings
	{
		public bool ShowGpuLoad { get; set; }

		public bool ShowCpuLoad { get; set; }

		public bool ShowCpuMaxThreadLoad { get; set; }

		public bool ShowGpuPowerLimit { get; set; }

        public bool ShowPcLatency { get; set; }

        public bool ShowAggregationSeparators { get; set; }

		public bool IsAnyPercentageGraphVisible { get; set; }

		public bool ShowThresholds { get; set; }

		public double StutteringFactor { get; set; }

		public double LowFPSThreshold { get; set; }
	}
}
