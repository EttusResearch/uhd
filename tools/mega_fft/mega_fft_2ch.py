#!/usr/bin/env python2
##################################################
# GNU Radio Python Flow Graph
# Copyright 2014 National Instruments
#
# Title: Mega FFT
# Description: Standard edition, dual-channel
# Generated: Sun Jul 26 20:09:53 2015
##################################################

if __name__ == '__main__':
    import ctypes
    import sys
    if sys.platform.startswith('linux'):
        try:
            x11 = ctypes.cdll.LoadLibrary('libX11.so')
            x11.XInitThreads()
        except:
            print "Warning: failed to XInitThreads()"

from gnuradio import analog
from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import gr
from gnuradio import uhd
from gnuradio import wxgui
from gnuradio.eng_option import eng_option
from gnuradio.fft import logpwrfft
from gnuradio.fft import window
from gnuradio.fft import window as fft_win
from gnuradio.filter import firdes
from gnuradio.wxgui import fftsink2
from gnuradio.wxgui import forms
from gnuradio.wxgui import scopesink2
from gnuradio.wxgui import waterfallsink2
from grc_gnuradio import blks2 as grc_blks2
from grc_gnuradio import wxgui as grc_wxgui
from optparse import OptionParser
import math
import threading
import time
import time, datetime
import wx

class mega_fft_2ch(grc_wxgui.top_block_gui):

    def __init__(self, antenna="", args="", ave=1*0 + 0.5, averaging="True", bw=0, dyn_rng=130, fft_rate=15, fft_ref_scale=2.0, fft_size=1024, freq=0 + 100e6, freq_fine_range=2e6, gain=float("-inf"), lo_check_interval=5, lo_offset=0, mag_alpha=1e-3, peak_hold="False", pps='', probe_interval=3, rate=1e6, ref='', ref_lvl=0, sensor_interval=2, show_stream_tags="False", spec='', stream_args="", window="auto", wire_format=""):
        grc_wxgui.top_block_gui.__init__(self, title="Mega FFT")

        ##################################################
        # Parameters
        ##################################################
        self.antenna = antenna
        self.args = args
        self.ave = ave
        self.averaging = averaging
        self.bw = bw
        self.dyn_rng = dyn_rng
        self.fft_rate = fft_rate
        self.fft_ref_scale = fft_ref_scale
        self.fft_size = fft_size
        self.freq = freq
        self.freq_fine_range = freq_fine_range
        self.gain = gain
        self.lo_check_interval = lo_check_interval
        self.lo_offset = lo_offset
        self.mag_alpha = mag_alpha
        self.peak_hold = peak_hold
        self.pps = pps
        self.probe_interval = probe_interval
        self.rate = rate
        self.ref = ref
        self.ref_lvl = ref_lvl
        self.sensor_interval = sensor_interval
        self.show_stream_tags = show_stream_tags
        self.spec = spec
        self.stream_args = stream_args
        self.window = window
        self.wire_format = wire_format

        ##################################################
        # Variables
        ##################################################
        self.relative_freq = relative_freq = 1
        self.gain_range = gain_range = uhd.gain_range(0,0,0)
        self.fft_max_idx = fft_max_idx = (fft_size/2)*0
        self.actual_sample_rate = actual_sample_rate = rate
        self.actual_center_freq = actual_center_freq = 0
        self.str_to_bool = str_to_bool = lambda x: len(x)>0 and x.lower()[0] not in ['0', 'n', 'f']
        self.signal_probe = signal_probe = 0.0
        self.samp_rate = samp_rate = int(actual_sample_rate)
        self.requested_freq_txt = requested_freq_txt = freq
        self.gain_default = gain_default = gain_range.start()
        self.freq_fine = freq_fine = 0
        self.fft_max_idx_norm = fft_max_idx_norm = [fft_max_idx,fft_max_idx-fft_size][int(fft_max_idx>(fft_size/2))]
        self.fft_center_freq = fft_center_freq = actual_center_freq * relative_freq
        self.antennas = antennas = ['']
        self.window_fn_name_map = window_fn_name_map = {'auto': 'Auto', 'bh': 'Blackman-Harris', 'ham': 'Hamming', 'han': 'Hanning', 'rect': 'Rectangular', 'flat': 'Flattop'}
        self.window_fn_map = window_fn_map = {'auto': None, 'bh': fft_win.blackmanharris, 'ham': fft_win.hamming, 'han': fft_win.hanning, 'rect': fft_win.rectangular, 'flat': fft_win.flattop}
        self.usrp_info = usrp_info = '(unknown)'
        self.tune_result = tune_result = uhd.tune_result_t()
        self.tune_mode = tune_mode = [1,0][lo_offset==0.0]
        self.time_probe = time_probe = uhd.time_spec_t()
        self.test = test = 1
        self.subdev_spec = subdev_spec = '(unknown)'
        self.signal_probe_log = signal_probe_log = math.log10([signal_probe,1.0][signal_probe==0.0])*10
        self.selected_gain = selected_gain = gain_default
        self.selected_antenna = selected_antenna = [ [antenna,antennas[0]][antenna not in antennas] ,antennas[0]][antenna=='']
        self.requested_sample_rate_base = requested_sample_rate_base = rate
        self.requested_freq = requested_freq = requested_freq_txt + freq_fine
        self.relative_time = relative_time = True
        self.motherboard_sensors = motherboard_sensors = '(none)'
        self.max_bin_freq = max_bin_freq = fft_center_freq + (((1.0*fft_max_idx_norm) / fft_size) * samp_rate)
        self.locked_probe = locked_probe = '(unknown)'
        self.lo_offset_txt = lo_offset_txt = lo_offset
        self.fft_peak_hold = fft_peak_hold = str_to_bool(peak_hold)
        self.fft_max_lvl_2 = fft_max_lvl_2 = 0.0
        self.fft_max_lvl = fft_max_lvl = 0.0
        self.fft_averaging = fft_averaging = str_to_bool(averaging)
        self.decim = decim = 1
        self.daughterboard_sensors = daughterboard_sensors = '(none)'
        self.clicked_freq = clicked_freq = 0
        self.window_fn = window_fn = window_fn_map[window]
        self.variable_static_usrp_info = variable_static_usrp_info = usrp_info
        self.variable_static_time_now = variable_static_time_now = str( [time.ctime(time_probe.get_real_secs()), datetime.timedelta(seconds=time_probe.get_real_secs()), time.gmtime(time_probe.get_real_secs())] [relative_time])
        self.variable_static_text_0_0 = variable_static_text_0_0 = daughterboard_sensors
        self.variable_static_text_0 = variable_static_text_0 = motherboard_sensors
        self.variable_static_subdev_spec = variable_static_subdev_spec = subdev_spec
        self.variable_static_rf_freq = variable_static_rf_freq = tune_result.actual_rf_freq
        self.variable_static_requested_freq = variable_static_requested_freq = requested_freq
        self.variable_static_max_bin_freq = variable_static_max_bin_freq = max_bin_freq
        self.variable_static_level_probe = variable_static_level_probe = signal_probe_log
        self.variable_static_fft_window_name = variable_static_fft_window_name = window_fn_name_map[window]
        self.variable_static_fft_max_lvl_log = variable_static_fft_max_lvl_log = fft_max_lvl
        self.variable_static_actual_sample_rate = variable_static_actual_sample_rate = actual_sample_rate
        self.variable_static_actual_dsp_freq = variable_static_actual_dsp_freq = tune_result.actual_dsp_freq
        self.variable_any_code_waterfall_ave = variable_any_code_waterfall_ave = fft_averaging
        self.variable_any_code_iq_correction = variable_any_code_iq_correction = None
        self.variable_any_code_fft_sink_peak_hold = variable_any_code_fft_sink_peak_hold = fft_peak_hold
        self.variable_any_code_fft_sink_ave = variable_any_code_fft_sink_ave = fft_averaging
        self.variable_any_code_auto_dc_offset_removal = variable_any_code_auto_dc_offset_removal = None
        self.update_time_source = update_time_source = None
        self.update_clock_source = update_clock_source = None
        self.tune_obj = tune_obj = [requested_freq, uhd.tune_request(requested_freq, lo_offset_txt), uhd.tune_request(requested_freq, dsp_freq=0, dsp_freq_policy=uhd.tune_request.POLICY_MANUAL)][tune_mode]
        self.static_locked = static_locked = '"' + str(locked_probe) + '"'
        self.show_stream_tags_chk = show_stream_tags_chk = [False, True][show_stream_tags.lower() != 'false']
        self.show_max_lvl = show_max_lvl = True
        self.show_max_freq = show_max_freq = True
        self.selected_gain_proxy = selected_gain_proxy = selected_gain
        self.selected_antenna_proxy = selected_antenna_proxy = selected_antenna
        self.scope_mode_2 = scope_mode_2 = 0+1
        self.scope_mode = scope_mode = 0+1
        self.requested_sample_rate = requested_sample_rate = requested_sample_rate_base / (1.*decim)
        self.motherboard_sensor_names = motherboard_sensor_names = []
        self.max_decim = max_decim = 256
        self.initial_gain = initial_gain = 0
        self.has_lo_locked = has_lo_locked = False
        self.freq_range = freq_range = uhd.freq_range(freq,freq+1)
        self.fix_invalid_freq = fix_invalid_freq = None
        self.fft_max_lvl_value_2 = fft_max_lvl_value_2 = fft_max_lvl_2
        self.fft_max_lvl_value = fft_max_lvl_value = fft_max_lvl
        self.fft_ave_probe = fft_ave_probe = ave
        self.daughterboard_sensor_names = daughterboard_sensor_names = []
        self.clicked_freq_txt = clicked_freq_txt = clicked_freq
        self.auto_iq_correction = auto_iq_correction = True
        self.auto_dc_offset_removal = auto_dc_offset_removal = True
        self.any_test_1 = any_test_1 = test

        ##################################################
        # Blocks
        ##################################################
        self.src = uhd.usrp_source(
        	",".join((args, "")),
        	uhd.stream_args(
        		cpu_format="fc32",
        		channels=range(2),
        	),

        )
        if spec != "": self.src.set_subdev_spec(spec, 0)
        self.src.set_samp_rate(requested_sample_rate)
        self.src.set_center_freq(uhd.tune_request(freq, lo_offset), 0)
        self.src.set_gain(selected_gain_proxy, 0)
        self.src.set_center_freq(uhd.tune_request(freq, lo_offset), 1)
        self.src.set_gain(selected_gain_proxy, 1)
        self.nb_right = self.nb_right = wx.Notebook(self.GetWin(), style=wx.NB_TOP)
        self.nb_right.AddPage(grc_wxgui.Panel(self.nb_right), "Params")
        self.GridAdd(self.nb_right, 0, 1, 1, 1)
        self.nb = self.nb = wx.Notebook(self.GetWin(), style=wx.NB_TOP)
        self.nb.AddPage(grc_wxgui.Panel(self.nb), "FFT")
        self.nb.AddPage(grc_wxgui.Panel(self.nb), "Scope")
        self.nb.AddPage(grc_wxgui.Panel(self.nb), "Waterfall")
        self.nb.AddPage(grc_wxgui.Panel(self.nb), "Sensors")
        self.GridAdd(self.nb, 0, 0, 1, 1)
        self.nb_test = self.nb_test = wx.Notebook(self.nb_right.GetPage(0).GetWin(), style=wx.NB_LEFT)
        self.nb_test.AddPage(grc_wxgui.Panel(self.nb_test), "Test")
        self.nb_right.GetPage(0).Add(self.nb_test)
        self.nb_analog = self.nb_analog = wx.Notebook(self.nb_right.GetPage(0).GetWin(), style=wx.NB_LEFT)
        self.nb_analog.AddPage(grc_wxgui.Panel(self.nb_analog), "Analog")
        self.nb_analog.AddPage(grc_wxgui.Panel(self.nb_analog), "DC")
        self.nb_right.GetPage(0).Add(self.nb_analog)
        if not hasattr(self, '_post_any_code_evaluators'):
        	self._post_any_code_evaluators = []
        	self.wxEVT_AnyCode = wxEVT_AnyCode = wx.NewEventType()
        	def _run_evaluators(event):
        		_post_any_code_evaluators = self._post_any_code_evaluators
        		if len(_post_any_code_evaluators) > 0:
        			for id, evaluator in _post_any_code_evaluators:
        				try:
        					evaluator()
        				except Exception, e:
        					print "Exception while running Any Code evaluator for '%s':" % (id), e
        			del  _post_any_code_evaluators[0:len(_post_any_code_evaluators)]
        	self._run_evaluators = _run_evaluators
        	try:
        		self.GetWin().Connect(-1, -1, wxEVT_AnyCode, _run_evaluators)
        	except:
        		pass	# FIXME
        	def _run_evaluators_later(evaluator=None):
        		if evaluator is not None:
        			self._post_any_code_evaluators += [evaluator]
        		try:
        			de = wx.PyEvent()
        			de.SetEventType(wxEVT_AnyCode)
        			wx.PostEvent(self.GetWin(), de)
        		except TypeError:
        			pass
        		except AttributeError:	# FIXME
        			print "Cannot post message"
        	self._run_evaluators_later = _run_evaluators_later
        	_run_evaluators_later()
        def __post_evalutate_gain_range():
        	self._post_any_code_evaluators += [('gain_range', lambda: self._evalutate_gain_range(**{}))]
        def __evalutate_gain_range(*args, **kwds):
        	try:
        		self.gain_range = self.src.get_gain_range()
        		self.set_gain_range(self.gain_range)
        	except AttributeError, e:
        		print "AttributeError while evaulating gain_range:", e
        		__post_evalutate_gain_range()
        	except Exception, e:
        		print "Exception while evaluating gain_range:", e
        self._evalutate_gain_range = __evalutate_gain_range
        self.__post_evalutate_gain_range = __post_evalutate_gain_range
        self._evalutate_gain_range(**{})
        gain_range = self.gain_range
        self.fft_sink = fftsink2.fft_sink_c(
        	self.nb.GetPage(0).GetWin(),
        	baseband_freq=fft_center_freq,
        	y_per_div=10,
        	y_divs=int(dyn_rng/10),
        	ref_level=ref_lvl,
        	ref_scale=fft_ref_scale,
        	sample_rate=samp_rate,
        	fft_size=1024*0 + fft_size,
        	fft_rate=fft_rate,
        	average=True,
        	avg_alpha=ave,
        	title="FFT Plot",
        	peak_hold=False,	fft_in=False,
        	always_run=False,
        	fft_out=False,
        )
        self.nb.GetPage(0).Add(self.fft_sink.win)
        def fft_sink_callback(x, y):
        	self.set_clicked_freq(x)

        self.fft_sink.set_callback(fft_sink_callback)
        self.waterfall_sink = waterfallsink2.waterfall_sink_c(
        	self.nb.GetPage(2).GetWin(),
        	baseband_freq=fft_center_freq,
        	dynamic_range=dyn_rng,
        	ref_level=ref_lvl,
        	ref_scale=fft_ref_scale,
        	sample_rate=samp_rate,
        	fft_size=512*0 + fft_size,
        	fft_rate=fft_rate,
        	average=True,
        	avg_alpha=ave,
        	title="Waterfall Plot",
        	fft_in=False,
        	always_run=False,
        	fft_out=False,
        )
        self.nb.GetPage(2).Add(self.waterfall_sink.win)
        def waterfall_sink_callback(x, y):
        	self.set_clicked_freq(x)

        self.waterfall_sink.set_callback(waterfall_sink_callback)
        self._test_chooser = forms.button(
        	parent=self.nb_test.GetPage(0).GetWin(),
        	value=self.test,
        	callback=self.set_test,
        	label="Test",
        	choices=[1],
        	labels=['Reset FFT average'],
        )
        self.nb_test.GetPage(0).Add(self._test_chooser)
        self._show_stream_tags_chk_check_box = forms.check_box(
        	parent=self.nb_test.GetPage(0).GetWin(),
        	value=self.show_stream_tags_chk,
        	callback=self.set_show_stream_tags_chk,
        	label="Print stream tags",
        	true=True,
        	false=False,
        )
        self.nb_test.GetPage(0).GridAdd(self._show_stream_tags_chk_check_box, 0, 0, 1, 1)
        self._scope_mode_2_chooser = forms.radio_buttons(
        	parent=self.nb.GetPage(1).GetWin(),
        	value=self.scope_mode_2,
        	callback=self.set_scope_mode_2,
        	label="Scope Mode 2",
        	choices=[0, 1],
        	labels=['Complex', 'Magnitude'],
        	style=wx.RA_HORIZONTAL,
        )
        self.nb.GetPage(1).Add(self._scope_mode_2_chooser)
        self._scope_mode_chooser = forms.radio_buttons(
        	parent=self.nb.GetPage(1).GetWin(),
        	value=self.scope_mode,
        	callback=self.set_scope_mode,
        	label="Scope Mode",
        	choices=[0, 1],
        	labels=['Complex', 'Magnitude'],
        	style=wx.RA_HORIZONTAL,
        )
        self.nb.GetPage(1).Add(self._scope_mode_chooser)
        self.probe_avg_mag = analog.probe_avg_mag_sqrd_c(0, mag_alpha)
        self.nb_rate = self.nb_rate = wx.Notebook(self.nb_right.GetPage(0).GetWin(), style=wx.NB_LEFT)
        self.nb_rate.AddPage(grc_wxgui.Panel(self.nb_rate), "Rate")
        self.nb_right.GetPage(0).Add(self.nb_rate)
        self.nb_info = self.nb_info = wx.Notebook(self.nb_right.GetPage(0).GetWin(), style=wx.NB_LEFT)
        self.nb_info.AddPage(grc_wxgui.Panel(self.nb_info), "Info")
        self.nb_right.GetPage(0).GridAdd(self.nb_info, 0, 0, 1, 1)
        self.nb_freq = self.nb_freq = wx.Notebook(self.nb_right.GetPage(0).GetWin(), style=wx.NB_LEFT)
        self.nb_freq.AddPage(grc_wxgui.Panel(self.nb_freq), "Freq")
        self.nb_right.GetPage(0).Add(self.nb_freq)
        if sensor_interval > 0:
            self._motherboard_sensor_names_poll_rate = sensor_interval
        else:
            self._motherboard_sensor_names_poll_rate = 1
        self._motherboard_sensor_names_enabled = sensor_interval > 0
        def _set_motherboard_sensor_names_poll_rate(rate):
            self._motherboard_sensor_names_enabled = rate > 0
            if rate > 0:
                self._motherboard_sensor_names_poll_rate = rate
        self.set_motherboard_sensor_names_poll_rate = _set_motherboard_sensor_names_poll_rate
        def _motherboard_sensor_names_probe():
            while True:
                if self._motherboard_sensor_names_enabled:
                    val = self.src.get_mboard_sensor_names()
                    try:
                        self.set_motherboard_sensor_names(val)
                    except AttributeError:
                        pass
                time.sleep(1.0/(self._motherboard_sensor_names_poll_rate))
        _motherboard_sensor_names_thread = threading.Thread(target=_motherboard_sensor_names_probe)
        _motherboard_sensor_names_thread.daemon = True
        _motherboard_sensor_names_thread.start()
        if not hasattr(self, '_post_any_code_evaluators'):
        	self._post_any_code_evaluators = []
        	self.wxEVT_AnyCode = wxEVT_AnyCode = wx.NewEventType()
        	def _run_evaluators(event):
        		_post_any_code_evaluators = self._post_any_code_evaluators
        		if len(_post_any_code_evaluators) > 0:
        			for id, evaluator in _post_any_code_evaluators:
        				try:
        					evaluator()
        				except Exception, e:
        					print "Exception while running Any Code evaluator for '%s':" % (id), e
        			del  _post_any_code_evaluators[0:len(_post_any_code_evaluators)]
        	self._run_evaluators = _run_evaluators
        	try:
        		self.GetWin().Connect(-1, -1, wxEVT_AnyCode, _run_evaluators)
        	except:
        		pass	# FIXME
        	def _run_evaluators_later(evaluator=None):
        		if evaluator is not None:
        			self._post_any_code_evaluators += [evaluator]
        		try:
        			de = wx.PyEvent()
        			de.SetEventType(wxEVT_AnyCode)
        			wx.PostEvent(self.GetWin(), de)
        		except TypeError:
        			pass
        		except AttributeError:	# FIXME
        			print "Cannot post message"
        	self._run_evaluators_later = _run_evaluators_later
        	_run_evaluators_later()
        def __post_evalutate_initial_gain():
        	self._post_any_code_evaluators += [('initial_gain', lambda: self._evalutate_initial_gain(**{'gain_range': gain_range}))]
        def __evalutate_initial_gain(*args, **kwds):
        	try:
        		self.initial_gain = [gain,gain_range.start() + ((gain_range.stop() - gain_range.start()) * 0.25)][gain==float('-inf')]
        		self.set_initial_gain(self.initial_gain)
        	except AttributeError, e:
        		print "AttributeError while evaulating initial_gain:", e
        		__post_evalutate_initial_gain()
        	except Exception, e:
        		print "Exception while evaluating initial_gain:", e
        self._evalutate_initial_gain = __evalutate_initial_gain
        self.__post_evalutate_initial_gain = __post_evalutate_initial_gain
        self._evalutate_initial_gain(**{'gain_range': gain_range})
        initial_gain = self.initial_gain
        if not hasattr(self, '_post_any_code_evaluators'):
        	self._post_any_code_evaluators = []
        	self.wxEVT_AnyCode = wxEVT_AnyCode = wx.NewEventType()
        	def _run_evaluators(event):
        		_post_any_code_evaluators = self._post_any_code_evaluators
        		if len(_post_any_code_evaluators) > 0:
        			for id, evaluator in _post_any_code_evaluators:
        				try:
        					evaluator()
        				except Exception, e:
        					print "Exception while running Any Code evaluator for '%s':" % (id), e
        			del  _post_any_code_evaluators[0:len(_post_any_code_evaluators)]
        	self._run_evaluators = _run_evaluators
        	try:
        		self.GetWin().Connect(-1, -1, wxEVT_AnyCode, _run_evaluators)
        	except:
        		pass	# FIXME
        	def _run_evaluators_later(evaluator=None):
        		if evaluator is not None:
        			self._post_any_code_evaluators += [evaluator]
        		try:
        			de = wx.PyEvent()
        			de.SetEventType(wxEVT_AnyCode)
        			wx.PostEvent(self.GetWin(), de)
        		except TypeError:
        			pass
        		except AttributeError:	# FIXME
        			print "Cannot post message"
        	self._run_evaluators_later = _run_evaluators_later
        	_run_evaluators_later()
        def __post_evalutate_has_lo_locked():
        	self._post_any_code_evaluators += [('has_lo_locked', lambda: self._evalutate_has_lo_locked(**{}))]
        def __evalutate_has_lo_locked(*args, **kwds):
        	try:
        		self.has_lo_locked = 'lo_locked' in self.src.get_sensor_names()
        		self.set_has_lo_locked(self.has_lo_locked)
        	except AttributeError, e:
        		print "AttributeError while evaulating has_lo_locked:", e
        		__post_evalutate_has_lo_locked()
        	except Exception, e:
        		print "Exception while evaluating has_lo_locked:", e
        self._evalutate_has_lo_locked = __evalutate_has_lo_locked
        self.__post_evalutate_has_lo_locked = __post_evalutate_has_lo_locked
        self._evalutate_has_lo_locked(**{})
        has_lo_locked = self.has_lo_locked
        if not hasattr(self, '_post_any_code_evaluators'):
        	self._post_any_code_evaluators = []
        	self.wxEVT_AnyCode = wxEVT_AnyCode = wx.NewEventType()
        	def _run_evaluators(event):
        		_post_any_code_evaluators = self._post_any_code_evaluators
        		if len(_post_any_code_evaluators) > 0:
        			for id, evaluator in _post_any_code_evaluators:
        				try:
        					evaluator()
        				except Exception, e:
        					print "Exception while running Any Code evaluator for '%s':" % (id), e
        			del  _post_any_code_evaluators[0:len(_post_any_code_evaluators)]
        	self._run_evaluators = _run_evaluators
        	try:
        		self.GetWin().Connect(-1, -1, wxEVT_AnyCode, _run_evaluators)
        	except:
        		pass	# FIXME
        	def _run_evaluators_later(evaluator=None):
        		if evaluator is not None:
        			self._post_any_code_evaluators += [evaluator]
        		try:
        			de = wx.PyEvent()
        			de.SetEventType(wxEVT_AnyCode)
        			wx.PostEvent(self.GetWin(), de)
        		except TypeError:
        			pass
        		except AttributeError:	# FIXME
        			print "Cannot post message"
        	self._run_evaluators_later = _run_evaluators_later
        	_run_evaluators_later()
        def __post_evalutate_freq_range():
        	self._post_any_code_evaluators += [('freq_range', lambda: self._evalutate_freq_range(**{}))]
        def __evalutate_freq_range(*args, **kwds):
        	try:
        		self.freq_range = self.src.get_freq_range()
        		self.set_freq_range(self.freq_range)
        	except AttributeError, e:
        		print "AttributeError while evaulating freq_range:", e
        		__post_evalutate_freq_range()
        	except Exception, e:
        		print "Exception while evaluating freq_range:", e
        self._evalutate_freq_range = __evalutate_freq_range
        self.__post_evalutate_freq_range = __post_evalutate_freq_range
        self._evalutate_freq_range(**{})
        freq_range = self.freq_range
        self.fft_max_lvl_probe_2 = blocks.probe_signal_f()
        if 5 > 0:
            self._fft_ave_probe_poll_rate = 5
        else:
            self._fft_ave_probe_poll_rate = 1
        self._fft_ave_probe_enabled = 5 > 0
        def _set_fft_ave_probe_poll_rate(rate):
            self._fft_ave_probe_enabled = rate > 0
            if rate > 0:
                self._fft_ave_probe_poll_rate = rate
        self.set_fft_ave_probe_poll_rate = _set_fft_ave_probe_poll_rate
        def _fft_ave_probe_probe():
            while True:
                if self._fft_ave_probe_enabled:
                    val = self.fft_sink.win['avg_alpha'] * [1.0/self.fft_sink.win['avg_alpha'],1.0][self.fft_sink.win['average']];()
                    try:
                        self.set_fft_ave_probe(val)
                    except AttributeError:
                        pass
                time.sleep(1.0/(self._fft_ave_probe_poll_rate))
        _fft_ave_probe_thread = threading.Thread(target=_fft_ave_probe_probe)
        _fft_ave_probe_thread.daemon = True
        _fft_ave_probe_thread.start()
        if sensor_interval > 0:
            self._daughterboard_sensor_names_poll_rate = sensor_interval
        else:
            self._daughterboard_sensor_names_poll_rate = 1
        self._daughterboard_sensor_names_enabled = sensor_interval > 0
        def _set_daughterboard_sensor_names_poll_rate(rate):
            self._daughterboard_sensor_names_enabled = rate > 0
            if rate > 0:
                self._daughterboard_sensor_names_poll_rate = rate
        self.set_daughterboard_sensor_names_poll_rate = _set_daughterboard_sensor_names_poll_rate
        def _daughterboard_sensor_names_probe():
            while True:
                if self._daughterboard_sensor_names_enabled:
                    val = self.src.get_sensor_names()
                    try:
                        self.set_daughterboard_sensor_names(val)
                    except AttributeError:
                        pass
                time.sleep(1.0/(self._daughterboard_sensor_names_poll_rate))
        _daughterboard_sensor_names_thread = threading.Thread(target=_daughterboard_sensor_names_probe)
        _daughterboard_sensor_names_thread.daemon = True
        _daughterboard_sensor_names_thread.start()
        self._auto_iq_correction_check_box = forms.check_box(
        	parent=self.nb_analog.GetPage(1).GetWin(),
        	value=self.auto_iq_correction,
        	callback=self.set_auto_iq_correction,
        	label="Auto IQ correction",
        	true=True,
        	false=False,
        )
        self.nb_analog.GetPage(1).Add(self._auto_iq_correction_check_box)
        self._auto_dc_offset_removal_check_box = forms.check_box(
        	parent=self.nb_analog.GetPage(1).GetWin(),
        	value=self.auto_dc_offset_removal,
        	callback=self.set_auto_dc_offset_removal,
        	label="Auto DC offset removal",
        	true=True,
        	false=False,
        )
        self.nb_analog.GetPage(1).Add(self._auto_dc_offset_removal_check_box)
        if not hasattr(self, '_post_any_code_evaluators'):
        	self._post_any_code_evaluators = []
        	self.wxEVT_AnyCode = wxEVT_AnyCode = wx.NewEventType()
        	def _run_evaluators(event):
        		_post_any_code_evaluators = self._post_any_code_evaluators
        		if len(_post_any_code_evaluators) > 0:
        			for id, evaluator in _post_any_code_evaluators:
        				try:
        					evaluator()
        				except Exception, e:
        					print "Exception while running Any Code evaluator for '%s':" % (id), e
        			del  _post_any_code_evaluators[0:len(_post_any_code_evaluators)]
        	self._run_evaluators = _run_evaluators
        	try:
        		self.GetWin().Connect(-1, -1, wxEVT_AnyCode, _run_evaluators)
        	except:
        		pass	# FIXME
        	def _run_evaluators_later(evaluator=None):
        		if evaluator is not None:
        			self._post_any_code_evaluators += [evaluator]
        		try:
        			de = wx.PyEvent()
        			de.SetEventType(wxEVT_AnyCode)
        			wx.PostEvent(self.GetWin(), de)
        		except TypeError:
        			pass
        		except AttributeError:	# FIXME
        			print "Cannot post message"
        	self._run_evaluators_later = _run_evaluators_later
        	_run_evaluators_later()
        def __post_evalutate_antennas():
        	self._post_any_code_evaluators += [('antennas', lambda: self._evalutate_antennas(**{}))]
        def __evalutate_antennas(*args, **kwds):
        	try:
        		self.antennas = self.src.get_antennas()
        		self.set_antennas(self.antennas)
        	except AttributeError, e:
        		print "AttributeError while evaulating antennas:", e
        		__post_evalutate_antennas()
        	except Exception, e:
        		print "Exception while evaluating antennas:", e
        self._evalutate_antennas = __evalutate_antennas
        self.__post_evalutate_antennas = __post_evalutate_antennas
        self._evalutate_antennas(**{})
        antennas = self.antennas
        self.wxgui_scopesink2_0_0 = scopesink2.scope_sink_c(
        	self.nb.GetPage(1).GetWin(),
        	title="Scope Plot 2",
        	sample_rate=samp_rate,
        	v_scale=0,
        	v_offset=0,
        	t_scale=0,
        	ac_couple=False,
        	xy_mode=False,
        	num_inputs=1,
        	trig_mode=wxgui.TRIG_MODE_AUTO,
        	y_axis_label="Counts",
        )
        self.nb.GetPage(1).Add(self.wxgui_scopesink2_0_0.win)
        self.wxgui_scopesink2_0 = scopesink2.scope_sink_c(
        	self.nb.GetPage(1).GetWin(),
        	title="Scope Plot",
        	sample_rate=samp_rate,
        	v_scale=0,
        	v_offset=0,
        	t_scale=0,
        	ac_couple=False,
        	xy_mode=False,
        	num_inputs=1,
        	trig_mode=wxgui.TRIG_MODE_AUTO,
        	y_axis_label="Counts",
        )
        self.nb.GetPage(1).Add(self.wxgui_scopesink2_0.win)
        self.waterfall_sink_0 = waterfallsink2.waterfall_sink_c(
        	self.nb.GetPage(2).GetWin(),
        	baseband_freq=fft_center_freq,
        	dynamic_range=dyn_rng,
        	ref_level=ref_lvl,
        	ref_scale=fft_ref_scale,
        	sample_rate=samp_rate,
        	fft_size=512*0 + fft_size,
        	fft_rate=fft_rate,
        	average=True,
        	avg_alpha=ave,
        	title="Waterfall Plot 2",
        	fft_in=False,
        	always_run=False,
        	fft_out=False,
        )
        self.nb.GetPage(2).Add(self.waterfall_sink_0.win)
        def waterfall_sink_0_callback(x, y):
        	self.set_clicked_freq(x)

        self.waterfall_sink_0.set_callback(waterfall_sink_0_callback)
        self._variable_static_usrp_info_static_text = forms.static_text(
        	parent=self.nb_info.GetPage(0).GetWin(),
        	value=self.variable_static_usrp_info,
        	callback=self.set_variable_static_usrp_info,
        	label="USRP",
        	converter=forms.str_converter(),
        )
        self.nb_info.GetPage(0).GridAdd(self._variable_static_usrp_info_static_text, 0, 0, 1, 10)
        self._variable_static_time_now_static_text = forms.static_text(
        	parent=self.nb_test.GetPage(0).GetWin(),
        	value=self.variable_static_time_now,
        	callback=self.set_variable_static_time_now,
        	label="Time now",
        	converter=forms.str_converter(),
        )
        self.nb_test.GetPage(0).GridAdd(self._variable_static_time_now_static_text, 1, 1, 1, 2)
        self._variable_static_text_0_0_static_text = forms.static_text(
        	parent=self.nb.GetPage(3).GetWin(),
        	value=self.variable_static_text_0_0,
        	callback=self.set_variable_static_text_0_0,
        	label="Daughterboard",
        	converter=forms.str_converter(),
        )
        self.nb.GetPage(3).GridAdd(self._variable_static_text_0_0_static_text, 2, 0, 1, 1)
        self._variable_static_text_0_static_text = forms.static_text(
        	parent=self.nb.GetPage(3).GetWin(),
        	value=self.variable_static_text_0,
        	callback=self.set_variable_static_text_0,
        	label="Motherboard",
        	converter=forms.str_converter(),
        )
        self.nb.GetPage(3).GridAdd(self._variable_static_text_0_static_text, 0, 0, 1, 1)
        self._variable_static_subdev_spec_static_text = forms.static_text(
        	parent=self.nb_info.GetPage(0).GetWin(),
        	value=self.variable_static_subdev_spec,
        	callback=self.set_variable_static_subdev_spec,
        	label="Sub-device",
        	converter=forms.str_converter(),
        )
        self.nb_info.GetPage(0).GridAdd(self._variable_static_subdev_spec_static_text, 1, 0, 1, 1)
        self._variable_static_rf_freq_static_text = forms.static_text(
        	parent=self.nb_freq.GetPage(0).GetWin(),
        	value=self.variable_static_rf_freq,
        	callback=self.set_variable_static_rf_freq,
        	label="Actual RF freq",
        	converter=forms.float_converter(),
        )
        self.nb_freq.GetPage(0).GridAdd(self._variable_static_rf_freq_static_text, 5, 0, 1, 1)
        self._variable_static_requested_freq_static_text = forms.static_text(
        	parent=self.nb_freq.GetPage(0).GetWin(),
        	value=self.variable_static_requested_freq,
        	callback=self.set_variable_static_requested_freq,
        	label="Requested base + fine freq",
        	converter=forms.float_converter(),
        )
        self.nb_freq.GetPage(0).GridAdd(self._variable_static_requested_freq_static_text, 3, 0, 1, 1)
        self._variable_static_max_bin_freq_static_text = forms.static_text(
        	parent=self.nb_test.GetPage(0).GetWin(),
        	value=self.variable_static_max_bin_freq,
        	callback=self.set_variable_static_max_bin_freq,
        	label="Peak freq",
        	converter=forms.float_converter(),
        )
        self.nb_test.GetPage(0).Add(self._variable_static_max_bin_freq_static_text)
        self._variable_static_level_probe_static_text = forms.static_text(
        	parent=self.nb_test.GetPage(0).GetWin(),
        	value=self.variable_static_level_probe,
        	callback=self.set_variable_static_level_probe,
        	label="Signal mag^2 (dB)",
        	converter=forms.float_converter(),
        )
        self.nb_test.GetPage(0).Add(self._variable_static_level_probe_static_text)
        self._variable_static_fft_window_name_static_text = forms.static_text(
        	parent=self.nb_info.GetPage(0).GetWin(),
        	value=self.variable_static_fft_window_name,
        	callback=self.set_variable_static_fft_window_name,
        	label="FFT window",
        	converter=forms.str_converter(),
        )
        self.nb_info.GetPage(0).Add(self._variable_static_fft_window_name_static_text)
        self._variable_static_fft_max_lvl_log_static_text = forms.static_text(
        	parent=self.nb_test.GetPage(0).GetWin(),
        	value=self.variable_static_fft_max_lvl_log,
        	callback=self.set_variable_static_fft_max_lvl_log,
        	label="Peak magnitude (dB)",
        	converter=forms.float_converter(),
        )
        self.nb_test.GetPage(0).Add(self._variable_static_fft_max_lvl_log_static_text)
        self._variable_static_actual_sample_rate_static_text = forms.static_text(
        	parent=self.nb_rate.GetPage(0).GetWin(),
        	value=self.variable_static_actual_sample_rate,
        	callback=self.set_variable_static_actual_sample_rate,
        	label="Actual",
        	converter=forms.float_converter(),
        )
        self.nb_rate.GetPage(0).GridAdd(self._variable_static_actual_sample_rate_static_text, 0, 1, 1, 1)
        self._variable_static_actual_dsp_freq_static_text = forms.static_text(
        	parent=self.nb_freq.GetPage(0).GetWin(),
        	value=self.variable_static_actual_dsp_freq,
        	callback=self.set_variable_static_actual_dsp_freq,
        	label="Actual DSP freq",
        	converter=forms.float_converter(),
        )
        self.nb_freq.GetPage(0).GridAdd(self._variable_static_actual_dsp_freq_static_text, 6, 0, 1, 1)
        if not hasattr(self, '_post_any_code_evaluators'):
        	self._post_any_code_evaluators = []
        	self.wxEVT_AnyCode = wxEVT_AnyCode = wx.NewEventType()
        	def _run_evaluators(event):
        		_post_any_code_evaluators = self._post_any_code_evaluators
        		if len(_post_any_code_evaluators) > 0:
        			for id, evaluator in _post_any_code_evaluators:
        				try:
        					evaluator()
        				except Exception, e:
        					print "Exception while running Any Code evaluator for '%s':" % (id), e
        			del  _post_any_code_evaluators[0:len(_post_any_code_evaluators)]
        	self._run_evaluators = _run_evaluators
        	try:
        		self.GetWin().Connect(-1, -1, wxEVT_AnyCode, _run_evaluators)
        	except:
        		pass	# FIXME
        	def _run_evaluators_later(evaluator=None):
        		if evaluator is not None:
        			self._post_any_code_evaluators += [evaluator]
        		try:
        			de = wx.PyEvent()
        			de.SetEventType(wxEVT_AnyCode)
        			wx.PostEvent(self.GetWin(), de)
        		except TypeError:
        			pass
        		except AttributeError:	# FIXME
        			print "Cannot post message"
        	self._run_evaluators_later = _run_evaluators_later
        	_run_evaluators_later()
        def __post_evalutate_variable_any_code_waterfall_ave():
        	self._post_any_code_evaluators += [('variable_any_code_waterfall_ave', lambda: self._evalutate_variable_any_code_waterfall_ave(**{'averaging': fft_averaging}))]
        def __evalutate_variable_any_code_waterfall_ave(*args, **kwds):
        	try:
        		self.variable_any_code_waterfall_ave = self.waterfall_sink.win['average'] = fft_averaging
        		self.set_variable_any_code_waterfall_ave(self.variable_any_code_waterfall_ave)
        	except AttributeError, e:
        		print "AttributeError while evaulating variable_any_code_waterfall_ave:", e
        		__post_evalutate_variable_any_code_waterfall_ave()
        	except Exception, e:
        		print "Exception while evaluating variable_any_code_waterfall_ave:", e
        self._evalutate_variable_any_code_waterfall_ave = __evalutate_variable_any_code_waterfall_ave
        self.__post_evalutate_variable_any_code_waterfall_ave = __post_evalutate_variable_any_code_waterfall_ave
        self.__post_evalutate_variable_any_code_waterfall_ave()
        if not hasattr(self, '_post_any_code_evaluators'):
        	self._post_any_code_evaluators = []
        	self.wxEVT_AnyCode = wxEVT_AnyCode = wx.NewEventType()
        	def _run_evaluators(event):
        		_post_any_code_evaluators = self._post_any_code_evaluators
        		if len(_post_any_code_evaluators) > 0:
        			for id, evaluator in _post_any_code_evaluators:
        				try:
        					evaluator()
        				except Exception, e:
        					print "Exception while running Any Code evaluator for '%s':" % (id), e
        			del  _post_any_code_evaluators[0:len(_post_any_code_evaluators)]
        	self._run_evaluators = _run_evaluators
        	try:
        		self.GetWin().Connect(-1, -1, wxEVT_AnyCode, _run_evaluators)
        	except:
        		pass	# FIXME
        	def _run_evaluators_later(evaluator=None):
        		if evaluator is not None:
        			self._post_any_code_evaluators += [evaluator]
        		try:
        			de = wx.PyEvent()
        			de.SetEventType(wxEVT_AnyCode)
        			wx.PostEvent(self.GetWin(), de)
        		except TypeError:
        			pass
        		except AttributeError:	# FIXME
        			print "Cannot post message"
        	self._run_evaluators_later = _run_evaluators_later
        	_run_evaluators_later()
        def __post_evalutate_variable_any_code_iq_correction():
        	self._post_any_code_evaluators += [('variable_any_code_iq_correction', lambda: self._evalutate_variable_any_code_iq_correction(**{'auto_iq_correction': auto_iq_correction}))]
        def __evalutate_variable_any_code_iq_correction(*args, **kwds):
        	try:
        		[self.src.set_auto_iq_balance(self.auto_iq_correction), self.auto_iq_correction or self.src.set_iq_balance(0+0j)]
        		self.set_variable_any_code_iq_correction(self.variable_any_code_iq_correction)
        	except AttributeError, e:
        		print "AttributeError while evaulating variable_any_code_iq_correction:", e
        		__post_evalutate_variable_any_code_iq_correction()
        	except Exception, e:
        		print "Exception while evaluating variable_any_code_iq_correction:", e
        self._evalutate_variable_any_code_iq_correction = __evalutate_variable_any_code_iq_correction
        self.__post_evalutate_variable_any_code_iq_correction = __post_evalutate_variable_any_code_iq_correction
        self._evalutate_variable_any_code_iq_correction(**{'auto_iq_correction': auto_iq_correction})
        variable_any_code_iq_correction = self.variable_any_code_iq_correction
        if not hasattr(self, '_post_any_code_evaluators'):
        	self._post_any_code_evaluators = []
        	self.wxEVT_AnyCode = wxEVT_AnyCode = wx.NewEventType()
        	def _run_evaluators(event):
        		_post_any_code_evaluators = self._post_any_code_evaluators
        		if len(_post_any_code_evaluators) > 0:
        			for id, evaluator in _post_any_code_evaluators:
        				try:
        					evaluator()
        				except Exception, e:
        					print "Exception while running Any Code evaluator for '%s':" % (id), e
        			del  _post_any_code_evaluators[0:len(_post_any_code_evaluators)]
        	self._run_evaluators = _run_evaluators
        	try:
        		self.GetWin().Connect(-1, -1, wxEVT_AnyCode, _run_evaluators)
        	except:
        		pass	# FIXME
        	def _run_evaluators_later(evaluator=None):
        		if evaluator is not None:
        			self._post_any_code_evaluators += [evaluator]
        		try:
        			de = wx.PyEvent()
        			de.SetEventType(wxEVT_AnyCode)
        			wx.PostEvent(self.GetWin(), de)
        		except TypeError:
        			pass
        		except AttributeError:	# FIXME
        			print "Cannot post message"
        	self._run_evaluators_later = _run_evaluators_later
        	_run_evaluators_later()
        def __post_evalutate_variable_any_code_fft_sink_peak_hold():
        	self._post_any_code_evaluators += [('variable_any_code_fft_sink_peak_hold', lambda: self._evalutate_variable_any_code_fft_sink_peak_hold(**{'peak_hold': fft_peak_hold}))]
        def __evalutate_variable_any_code_fft_sink_peak_hold(*args, **kwds):
        	try:
        		self.variable_any_code_fft_sink_peak_hold = self.fft_sink.win['peak_hold'] = fft_peak_hold
        		self.set_variable_any_code_fft_sink_peak_hold(self.variable_any_code_fft_sink_peak_hold)
        	except AttributeError, e:
        		print "AttributeError while evaulating variable_any_code_fft_sink_peak_hold:", e
        		__post_evalutate_variable_any_code_fft_sink_peak_hold()
        	except Exception, e:
        		print "Exception while evaluating variable_any_code_fft_sink_peak_hold:", e
        self._evalutate_variable_any_code_fft_sink_peak_hold = __evalutate_variable_any_code_fft_sink_peak_hold
        self.__post_evalutate_variable_any_code_fft_sink_peak_hold = __post_evalutate_variable_any_code_fft_sink_peak_hold
        self._evalutate_variable_any_code_fft_sink_peak_hold(**{'peak_hold': fft_peak_hold})
        variable_any_code_fft_sink_peak_hold = self.variable_any_code_fft_sink_peak_hold
        if not hasattr(self, '_post_any_code_evaluators'):
        	self._post_any_code_evaluators = []
        	self.wxEVT_AnyCode = wxEVT_AnyCode = wx.NewEventType()
        	def _run_evaluators(event):
        		_post_any_code_evaluators = self._post_any_code_evaluators
        		if len(_post_any_code_evaluators) > 0:
        			for id, evaluator in _post_any_code_evaluators:
        				try:
        					evaluator()
        				except Exception, e:
        					print "Exception while running Any Code evaluator for '%s':" % (id), e
        			del  _post_any_code_evaluators[0:len(_post_any_code_evaluators)]
        	self._run_evaluators = _run_evaluators
        	try:
        		self.GetWin().Connect(-1, -1, wxEVT_AnyCode, _run_evaluators)
        	except:
        		pass	# FIXME
        	def _run_evaluators_later(evaluator=None):
        		if evaluator is not None:
        			self._post_any_code_evaluators += [evaluator]
        		try:
        			de = wx.PyEvent()
        			de.SetEventType(wxEVT_AnyCode)
        			wx.PostEvent(self.GetWin(), de)
        		except TypeError:
        			pass
        		except AttributeError:	# FIXME
        			print "Cannot post message"
        	self._run_evaluators_later = _run_evaluators_later
        	_run_evaluators_later()
        def __post_evalutate_variable_any_code_fft_sink_ave():
        	self._post_any_code_evaluators += [('variable_any_code_fft_sink_ave', lambda: self._evalutate_variable_any_code_fft_sink_ave(**{'averaging': fft_averaging}))]
        def __evalutate_variable_any_code_fft_sink_ave(*args, **kwds):
        	try:
        		self.variable_any_code_fft_sink_ave = self.fft_sink.win['average'] = fft_averaging
        		self.set_variable_any_code_fft_sink_ave(self.variable_any_code_fft_sink_ave)
        	except AttributeError, e:
        		print "AttributeError while evaulating variable_any_code_fft_sink_ave:", e
        		__post_evalutate_variable_any_code_fft_sink_ave()
        	except Exception, e:
        		print "Exception while evaluating variable_any_code_fft_sink_ave:", e
        self._evalutate_variable_any_code_fft_sink_ave = __evalutate_variable_any_code_fft_sink_ave
        self.__post_evalutate_variable_any_code_fft_sink_ave = __post_evalutate_variable_any_code_fft_sink_ave
        self.__post_evalutate_variable_any_code_fft_sink_ave()
        if not hasattr(self, '_post_any_code_evaluators'):
        	self._post_any_code_evaluators = []
        	self.wxEVT_AnyCode = wxEVT_AnyCode = wx.NewEventType()
        	def _run_evaluators(event):
        		_post_any_code_evaluators = self._post_any_code_evaluators
        		if len(_post_any_code_evaluators) > 0:
        			for id, evaluator in _post_any_code_evaluators:
        				try:
        					evaluator()
        				except Exception, e:
        					print "Exception while running Any Code evaluator for '%s':" % (id), e
        			del  _post_any_code_evaluators[0:len(_post_any_code_evaluators)]
        	self._run_evaluators = _run_evaluators
        	try:
        		self.GetWin().Connect(-1, -1, wxEVT_AnyCode, _run_evaluators)
        	except:
        		pass	# FIXME
        	def _run_evaluators_later(evaluator=None):
        		if evaluator is not None:
        			self._post_any_code_evaluators += [evaluator]
        		try:
        			de = wx.PyEvent()
        			de.SetEventType(wxEVT_AnyCode)
        			wx.PostEvent(self.GetWin(), de)
        		except TypeError:
        			pass
        		except AttributeError:	# FIXME
        			print "Cannot post message"
        	self._run_evaluators_later = _run_evaluators_later
        	_run_evaluators_later()
        def __post_evalutate_variable_any_code_auto_dc_offset_removal():
        	self._post_any_code_evaluators += [('variable_any_code_auto_dc_offset_removal', lambda: self._evalutate_variable_any_code_auto_dc_offset_removal(**{'auto_dc_offset_removal': auto_dc_offset_removal}))]
        def __evalutate_variable_any_code_auto_dc_offset_removal(*args, **kwds):
        	try:
        		[self.src.set_auto_dc_offset(self.auto_dc_offset_removal), self.auto_dc_offset_removal or self.src.set_dc_offset(0+0j)]
        		self.set_variable_any_code_auto_dc_offset_removal(self.variable_any_code_auto_dc_offset_removal)
        	except AttributeError, e:
        		print "AttributeError while evaulating variable_any_code_auto_dc_offset_removal:", e
        		__post_evalutate_variable_any_code_auto_dc_offset_removal()
        	except Exception, e:
        		print "Exception while evaluating variable_any_code_auto_dc_offset_removal:", e
        self._evalutate_variable_any_code_auto_dc_offset_removal = __evalutate_variable_any_code_auto_dc_offset_removal
        self.__post_evalutate_variable_any_code_auto_dc_offset_removal = __post_evalutate_variable_any_code_auto_dc_offset_removal
        self._evalutate_variable_any_code_auto_dc_offset_removal(**{'auto_dc_offset_removal': auto_dc_offset_removal})
        variable_any_code_auto_dc_offset_removal = self.variable_any_code_auto_dc_offset_removal
        if not hasattr(self, '_post_any_code_evaluators'):
        	self._post_any_code_evaluators = []
        	self.wxEVT_AnyCode = wxEVT_AnyCode = wx.NewEventType()
        	def _run_evaluators(event):
        		_post_any_code_evaluators = self._post_any_code_evaluators
        		if len(_post_any_code_evaluators) > 0:
        			for id, evaluator in _post_any_code_evaluators:
        				try:
        					evaluator()
        				except Exception, e:
        					print "Exception while running Any Code evaluator for '%s':" % (id), e
        			del  _post_any_code_evaluators[0:len(_post_any_code_evaluators)]
        	self._run_evaluators = _run_evaluators
        	try:
        		self.GetWin().Connect(-1, -1, wxEVT_AnyCode, _run_evaluators)
        	except:
        		pass	# FIXME
        	def _run_evaluators_later(evaluator=None):
        		if evaluator is not None:
        			self._post_any_code_evaluators += [evaluator]
        		try:
        			de = wx.PyEvent()
        			de.SetEventType(wxEVT_AnyCode)
        			wx.PostEvent(self.GetWin(), de)
        		except TypeError:
        			pass
        		except AttributeError:	# FIXME
        			print "Cannot post message"
        	self._run_evaluators_later = _run_evaluators_later
        	_run_evaluators_later()
        def __post_evalutate_usrp_info():
        	self._post_any_code_evaluators += [('usrp_info', lambda: self._evalutate_usrp_info(**{}))]
        def __evalutate_usrp_info(*args, **kwds):
        	try:
        		self.usrp_info = '%s (\'%s\'), %s' % (self.src.get_usrp_info().get('mboard_id'), self.src.get_usrp_info().get('mboard_name'), self.src.get_usrp_info().get('rx_subdev_name'))
        		self.set_usrp_info(self.usrp_info)
        	except AttributeError, e:
        		print "AttributeError while evaulating usrp_info:", e
        		__post_evalutate_usrp_info()
        	except Exception, e:
        		print "Exception while evaluating usrp_info:", e
        self._evalutate_usrp_info = __evalutate_usrp_info
        self.__post_evalutate_usrp_info = __post_evalutate_usrp_info
        self._evalutate_usrp_info(**{})
        usrp_info = self.usrp_info
        if not hasattr(self, '_post_any_code_evaluators'):
        	self._post_any_code_evaluators = []
        	self.wxEVT_AnyCode = wxEVT_AnyCode = wx.NewEventType()
        	def _run_evaluators(event):
        		_post_any_code_evaluators = self._post_any_code_evaluators
        		if len(_post_any_code_evaluators) > 0:
        			for id, evaluator in _post_any_code_evaluators:
        				try:
        					evaluator()
        				except Exception, e:
        					print "Exception while running Any Code evaluator for '%s':" % (id), e
        			del  _post_any_code_evaluators[0:len(_post_any_code_evaluators)]
        	self._run_evaluators = _run_evaluators
        	try:
        		self.GetWin().Connect(-1, -1, wxEVT_AnyCode, _run_evaluators)
        	except:
        		pass	# FIXME
        	def _run_evaluators_later(evaluator=None):
        		if evaluator is not None:
        			self._post_any_code_evaluators += [evaluator]
        		try:
        			de = wx.PyEvent()
        			de.SetEventType(wxEVT_AnyCode)
        			wx.PostEvent(self.GetWin(), de)
        		except TypeError:
        			pass
        		except AttributeError:	# FIXME
        			print "Cannot post message"
        	self._run_evaluators_later = _run_evaluators_later
        	_run_evaluators_later()
        def __post_evalutate_update_time_source():
        	self._post_any_code_evaluators += [('update_time_source', lambda: self._evalutate_update_time_source(**{'pps': pps}))]
        def __evalutate_update_time_source(*args, **kwds):
        	try:
        		if self.pps != '': self.src.set_time_source(self.pps, 0); print '1 PPS =', self.pps;
        		self.set_update_time_source(self.update_time_source)
        	except AttributeError, e:
        		print "AttributeError while evaulating update_time_source:", e
        		__post_evalutate_update_time_source()
        	except Exception, e:
        		print "Exception while evaluating update_time_source:", e
        self._evalutate_update_time_source = __evalutate_update_time_source
        self.__post_evalutate_update_time_source = __post_evalutate_update_time_source
        self._evalutate_update_time_source(**{'pps': pps})
        update_time_source = self.update_time_source
        if not hasattr(self, '_post_any_code_evaluators'):
        	self._post_any_code_evaluators = []
        	self.wxEVT_AnyCode = wxEVT_AnyCode = wx.NewEventType()
        	def _run_evaluators(event):
        		_post_any_code_evaluators = self._post_any_code_evaluators
        		if len(_post_any_code_evaluators) > 0:
        			for id, evaluator in _post_any_code_evaluators:
        				try:
        					evaluator()
        				except Exception, e:
        					print "Exception while running Any Code evaluator for '%s':" % (id), e
        			del  _post_any_code_evaluators[0:len(_post_any_code_evaluators)]
        	self._run_evaluators = _run_evaluators
        	try:
        		self.GetWin().Connect(-1, -1, wxEVT_AnyCode, _run_evaluators)
        	except:
        		pass	# FIXME
        	def _run_evaluators_later(evaluator=None):
        		if evaluator is not None:
        			self._post_any_code_evaluators += [evaluator]
        		try:
        			de = wx.PyEvent()
        			de.SetEventType(wxEVT_AnyCode)
        			wx.PostEvent(self.GetWin(), de)
        		except TypeError:
        			pass
        		except AttributeError:	# FIXME
        			print "Cannot post message"
        	self._run_evaluators_later = _run_evaluators_later
        	_run_evaluators_later()
        def __post_evalutate_update_clock_source():
        	self._post_any_code_evaluators += [('update_clock_source', lambda: self._evalutate_update_clock_source(**{'ref': ref}))]
        def __evalutate_update_clock_source(*args, **kwds):
        	try:
        		if self.ref != '': self.src.set_clock_source(self.ref, 0); print 'Ref =', self.ref;
        		self.set_update_clock_source(self.update_clock_source)
        	except AttributeError, e:
        		print "AttributeError while evaulating update_clock_source:", e
        		__post_evalutate_update_clock_source()
        	except Exception, e:
        		print "Exception while evaluating update_clock_source:", e
        self._evalutate_update_clock_source = __evalutate_update_clock_source
        self.__post_evalutate_update_clock_source = __post_evalutate_update_clock_source
        self._evalutate_update_clock_source(**{'ref': ref})
        update_clock_source = self.update_clock_source
        if not hasattr(self, '_post_any_code_evaluators'):
        	self._post_any_code_evaluators = []
        	self.wxEVT_AnyCode = wxEVT_AnyCode = wx.NewEventType()
        	def _run_evaluators(event):
        		_post_any_code_evaluators = self._post_any_code_evaluators
        		if len(_post_any_code_evaluators) > 0:
        			for id, evaluator in _post_any_code_evaluators:
        				try:
        					evaluator()
        				except Exception, e:
        					print "Exception while running Any Code evaluator for '%s':" % (id), e
        			del  _post_any_code_evaluators[0:len(_post_any_code_evaluators)]
        	self._run_evaluators = _run_evaluators
        	try:
        		self.GetWin().Connect(-1, -1, wxEVT_AnyCode, _run_evaluators)
        	except:
        		pass	# FIXME
        	def _run_evaluators_later(evaluator=None):
        		if evaluator is not None:
        			self._post_any_code_evaluators += [evaluator]
        		try:
        			de = wx.PyEvent()
        			de.SetEventType(wxEVT_AnyCode)
        			wx.PostEvent(self.GetWin(), de)
        		except TypeError:
        			pass
        		except AttributeError:	# FIXME
        			print "Cannot post message"
        	self._run_evaluators_later = _run_evaluators_later
        	_run_evaluators_later()
        def __post_evalutate_tune_result():
        	self._post_any_code_evaluators += [('tune_result', lambda: self._evalutate_tune_result(**{'tune_obj': tune_obj}))]
        def __evalutate_tune_result(*args, **kwds):
        	try:
        		self.tune_result = self.src.set_center_freq(self.tune_obj)
        		self.set_tune_result(self.tune_result)
        	except AttributeError, e:
        		print "AttributeError while evaulating tune_result:", e
        		__post_evalutate_tune_result()
        	except Exception, e:
        		print "Exception while evaluating tune_result:", e
        self._evalutate_tune_result = __evalutate_tune_result
        self.__post_evalutate_tune_result = __post_evalutate_tune_result
        self._evalutate_tune_result(**{'tune_obj': tune_obj})
        tune_result = self.tune_result
        self._tune_mode_chooser = forms.drop_down(
        	parent=self.nb_freq.GetPage(0).GetWin(),
        	value=self.tune_mode,
        	callback=self.set_tune_mode,
        	label="Tune mode",
        	choices=[0, 1, 2],
        	labels=['Auto (no LO offset)', 'Auto with LO offset', 'Manual (no DSP)'],
        )
        self.nb_freq.GetPage(0).GridAdd(self._tune_mode_chooser, 0, 0, 1, 1)
        if sensor_interval > 0:
            self._time_probe_poll_rate = sensor_interval
        else:
            self._time_probe_poll_rate = 1
        self._time_probe_enabled = sensor_interval > 0
        def _set_time_probe_poll_rate(rate):
            self._time_probe_enabled = rate > 0
            if rate > 0:
                self._time_probe_poll_rate = rate
        self.set_time_probe_poll_rate = _set_time_probe_poll_rate
        def _time_probe_probe():
            while True:
                if self._time_probe_enabled:
                    val = self.src.get_time_now()
                    try:
                        self.set_time_probe(val)
                    except AttributeError:
                        pass
                time.sleep(1.0/(self._time_probe_poll_rate))
        _time_probe_thread = threading.Thread(target=_time_probe_probe)
        _time_probe_thread.daemon = True
        _time_probe_thread.start()
        if not hasattr(self, '_post_any_code_evaluators'):
        	self._post_any_code_evaluators = []
        	self.wxEVT_AnyCode = wxEVT_AnyCode = wx.NewEventType()
        	def _run_evaluators(event):
        		_post_any_code_evaluators = self._post_any_code_evaluators
        		if len(_post_any_code_evaluators) > 0:
        			for id, evaluator in _post_any_code_evaluators:
        				try:
        					evaluator()
        				except Exception, e:
        					print "Exception while running Any Code evaluator for '%s':" % (id), e
        			del  _post_any_code_evaluators[0:len(_post_any_code_evaluators)]
        	self._run_evaluators = _run_evaluators
        	try:
        		self.GetWin().Connect(-1, -1, wxEVT_AnyCode, _run_evaluators)
        	except:
        		pass	# FIXME
        	def _run_evaluators_later(evaluator=None):
        		if evaluator is not None:
        			self._post_any_code_evaluators += [evaluator]
        		try:
        			de = wx.PyEvent()
        			de.SetEventType(wxEVT_AnyCode)
        			wx.PostEvent(self.GetWin(), de)
        		except TypeError:
        			pass
        		except AttributeError:	# FIXME
        			print "Cannot post message"
        	self._run_evaluators_later = _run_evaluators_later
        	_run_evaluators_later()
        def __post_evalutate_subdev_spec():
        	self._post_any_code_evaluators += [('subdev_spec', lambda: self._evalutate_subdev_spec(**{}))]
        def __evalutate_subdev_spec(*args, **kwds):
        	try:
        		self.subdev_spec = '[' + self.src.get_subdev_spec().strip() + ']'
        		self.set_subdev_spec(self.subdev_spec)
        	except AttributeError, e:
        		print "AttributeError while evaulating subdev_spec:", e
        		__post_evalutate_subdev_spec()
        	except Exception, e:
        		print "Exception while evaluating subdev_spec:", e
        self._evalutate_subdev_spec = __evalutate_subdev_spec
        self.__post_evalutate_subdev_spec = __post_evalutate_subdev_spec
        self._evalutate_subdev_spec(**{})
        subdev_spec = self.subdev_spec
        self._static_locked_static_text = forms.static_text(
        	parent=self.nb_info.GetPage(0).GetWin(),
        	value=self.static_locked,
        	callback=self.set_static_locked,
        	label="Locked",
        	converter=forms.str_converter(),
        )
        self.nb_info.GetPage(0).Add(self._static_locked_static_text)
        if probe_interval > 0:
            self._signal_probe_poll_rate = probe_interval
        else:
            self._signal_probe_poll_rate = 1
        self._signal_probe_enabled = probe_interval > 0
        def _set_signal_probe_poll_rate(rate):
            self._signal_probe_enabled = rate > 0
            if rate > 0:
                self._signal_probe_poll_rate = rate
        self.set_signal_probe_poll_rate = _set_signal_probe_poll_rate
        def _signal_probe_probe():
            while True:
                if self._signal_probe_enabled:
                    val = self.probe_avg_mag.level()
                    try:
                        self.set_signal_probe(val)
                    except AttributeError:
                        pass
                time.sleep(1.0/(self._signal_probe_poll_rate))
        _signal_probe_thread = threading.Thread(target=_signal_probe_probe)
        _signal_probe_thread.daemon = True
        _signal_probe_thread.start()
        self._show_max_lvl_check_box = forms.check_box(
        	parent=self.nb_test.GetPage(0).GetWin(),
        	value=self.show_max_lvl,
        	callback=self.set_show_max_lvl,
        	label="Show max level",
        	true=True,
        	false=False,
        )
        self.nb_test.GetPage(0).GridAdd(self._show_max_lvl_check_box, 0, 2, 1, 1)
        self._show_max_freq_check_box = forms.check_box(
        	parent=self.nb_test.GetPage(0).GetWin(),
        	value=self.show_max_freq,
        	callback=self.set_show_max_freq,
        	label="Show max freq",
        	true=True,
        	false=False,
        )
        self.nb_test.GetPage(0).GridAdd(self._show_max_freq_check_box, 0, 1, 1, 1)
        _selected_gain_sizer = wx.BoxSizer(wx.VERTICAL)
        self._selected_gain_text_box = forms.text_box(
        	parent=self.nb_analog.GetPage(0).GetWin(),
        	sizer=_selected_gain_sizer,
        	value=self.selected_gain,
        	callback=self.set_selected_gain,
        	label="Gain",
        	converter=forms.float_converter(),
        	proportion=0,
        )
        self._selected_gain_slider = forms.slider(
        	parent=self.nb_analog.GetPage(0).GetWin(),
        	sizer=_selected_gain_sizer,
        	value=self.selected_gain,
        	callback=self.set_selected_gain,
        	minimum=gain_range.start(),
        	maximum=[gain_range.stop(), gain_range.start() + 1.0][gain_range.stop()==gain_range.start()],
        	num_steps=[int((abs(gain_range.stop()-gain_range.start())/[gain_range.step(), 1.0][gain_range.step()==0])), 1][gain_range.stop()==gain_range.start()],
        	style=wx.SL_HORIZONTAL,
        	cast=float,
        	proportion=1,
        )
        self.nb_analog.GetPage(0).Add(_selected_gain_sizer)
        self._selected_antenna_chooser = forms.radio_buttons(
        	parent=self.nb_analog.GetPage(0).GetWin(),
        	value=self.selected_antenna,
        	callback=self.set_selected_antenna,
        	label="Antenna",
        	choices=antennas,
        	labels=[antennas,['(default)']][antennas==('',)],
        	style=wx.RA_HORIZONTAL,
        )
        self.nb_analog.GetPage(0).Add(self._selected_antenna_chooser)
        self._requested_sample_rate_base_text_box = forms.text_box(
        	parent=self.nb_rate.GetPage(0).GetWin(),
        	value=self.requested_sample_rate_base,
        	callback=self.set_requested_sample_rate_base,
        	label="Requested base sample rate",
        	converter=forms.float_converter(),
        )
        self.nb_rate.GetPage(0).GridAdd(self._requested_sample_rate_base_text_box, 0, 0, 1, 1)
        _requested_freq_txt_sizer = wx.BoxSizer(wx.VERTICAL)
        self._requested_freq_txt_text_box = forms.text_box(
        	parent=self.nb_freq.GetPage(0).GetWin(),
        	sizer=_requested_freq_txt_sizer,
        	value=self.requested_freq_txt,
        	callback=self.set_requested_freq_txt,
        	label="Requested freq",
        	converter=forms.float_converter(),
        	proportion=0,
        )
        self._requested_freq_txt_slider = forms.slider(
        	parent=self.nb_freq.GetPage(0).GetWin(),
        	sizer=_requested_freq_txt_sizer,
        	value=self.requested_freq_txt,
        	callback=self.set_requested_freq_txt,
        	minimum=freq_range.start(),
        	maximum=[freq_range.stop(), freq_range.start() + 1.0][freq_range.start()==freq_range.stop()],
        	num_steps=[1000,1][freq_range.start()==freq_range.stop()],
        	style=wx.SL_HORIZONTAL,
        	cast=float,
        	proportion=1,
        )
        self.nb_freq.GetPage(0).GridAdd(_requested_freq_txt_sizer, 1, 0, 1, 1)
        self._relative_time_chooser = forms.drop_down(
        	parent=self.nb_test.GetPage(0).GetWin(),
        	value=self.relative_time,
        	callback=self.set_relative_time,
        	label="Time display",
        	choices=[False, True],
        	labels=['Absolute', 'Relative'],
        )
        self.nb_test.GetPage(0).GridAdd(self._relative_time_chooser, 1, 0, 1, 1)
        self._relative_freq_chooser = forms.drop_down(
        	parent=self.nb_freq.GetPage(0).GetWin(),
        	value=self.relative_freq,
        	callback=self.set_relative_freq,
        	label="Frequency Axis",
        	choices=[1, 0],
        	labels=['RF', 'Baseband'],
        )
        self.nb_freq.GetPage(0).GridAdd(self._relative_freq_chooser, 7, 0, 1, 1)
        if not hasattr(self, '_post_any_code_evaluators'):
        	self._post_any_code_evaluators = []
        	self.wxEVT_AnyCode = wxEVT_AnyCode = wx.NewEventType()
        	def _run_evaluators(event):
        		_post_any_code_evaluators = self._post_any_code_evaluators
        		if len(_post_any_code_evaluators) > 0:
        			for id, evaluator in _post_any_code_evaluators:
        				try:
        					evaluator()
        				except Exception, e:
        					print "Exception while running Any Code evaluator for '%s':" % (id), e
        			del  _post_any_code_evaluators[0:len(_post_any_code_evaluators)]
        	self._run_evaluators = _run_evaluators
        	try:
        		self.GetWin().Connect(-1, -1, wxEVT_AnyCode, _run_evaluators)
        	except:
        		pass	# FIXME
        	def _run_evaluators_later(evaluator=None):
        		if evaluator is not None:
        			self._post_any_code_evaluators += [evaluator]
        		try:
        			de = wx.PyEvent()
        			de.SetEventType(wxEVT_AnyCode)
        			wx.PostEvent(self.GetWin(), de)
        		except TypeError:
        			pass
        		except AttributeError:	# FIXME
        			print "Cannot post message"
        	self._run_evaluators_later = _run_evaluators_later
        	_run_evaluators_later()
        def __post_evalutate_motherboard_sensors():
        	self._post_any_code_evaluators += [('motherboard_sensors', lambda: self._evalutate_motherboard_sensors(**{'motherboard_sensor_names': motherboard_sensor_names}))]
        def __evalutate_motherboard_sensors(*args, **kwds):
        	try:
        		self.motherboard_sensors = '\n'.join(map(lambda x: '%s: %s' % (x, str(self.src.get_mboard_sensor(x))), filter(lambda x: x.find('gps') != 0, self.motherboard_sensor_names)))
        		self.set_motherboard_sensors(self.motherboard_sensors)
        	except AttributeError, e:
        		print "AttributeError while evaulating motherboard_sensors:", e
        		__post_evalutate_motherboard_sensors()
        	except Exception, e:
        		print "Exception while evaluating motherboard_sensors:", e
        self._evalutate_motherboard_sensors = __evalutate_motherboard_sensors
        self.__post_evalutate_motherboard_sensors = __post_evalutate_motherboard_sensors
        self._evalutate_motherboard_sensors(**{'motherboard_sensor_names': motherboard_sensor_names})
        motherboard_sensors = self.motherboard_sensors
        self.logpwrfft_x_0_0 = logpwrfft.logpwrfft_c(
        	sample_rate=samp_rate,
        	fft_size=fft_size,
        	ref_scale=fft_ref_scale,
        	frame_rate=fft_rate,
        	avg_alpha=fft_ave_probe,
        	average=False,
        )
        self.logpwrfft_x_0 = logpwrfft.logpwrfft_c(
        	sample_rate=samp_rate,
        	fft_size=fft_size,
        	ref_scale=fft_ref_scale,
        	frame_rate=fft_rate,
        	avg_alpha=fft_ave_probe,
        	average=False,
        )
        if lo_check_interval * float(has_lo_locked) > 0:
            self._locked_probe_poll_rate = lo_check_interval * float(has_lo_locked)
        else:
            self._locked_probe_poll_rate = 1
        self._locked_probe_enabled = lo_check_interval * float(has_lo_locked) > 0
        def _set_locked_probe_poll_rate(rate):
            self._locked_probe_enabled = rate > 0
            if rate > 0:
                self._locked_probe_poll_rate = rate
        self.set_locked_probe_poll_rate = _set_locked_probe_poll_rate
        def _locked_probe_probe():
            while True:
                if self._locked_probe_enabled:
                    val = self.src.get_sensor('lo_locked')
                    try:
                        self.set_locked_probe(val)
                    except AttributeError:
                        pass
                time.sleep(1.0/(self._locked_probe_poll_rate))
        _locked_probe_thread = threading.Thread(target=_locked_probe_probe)
        _locked_probe_thread.daemon = True
        _locked_probe_thread.start()
        self._lo_offset_txt_text_box = forms.text_box(
        	parent=self.nb_freq.GetPage(0).GetWin(),
        	value=self.lo_offset_txt,
        	callback=self.set_lo_offset_txt,
        	label="LO offset (only for LO offset tuning mode)",
        	converter=forms.float_converter(),
        )
        self.nb_freq.GetPage(0).GridAdd(self._lo_offset_txt_text_box, 4, 0, 1, 1)
        if not hasattr(self, '_post_any_code_evaluators'):
        	self._post_any_code_evaluators = []
        	self.wxEVT_AnyCode = wxEVT_AnyCode = wx.NewEventType()
        	def _run_evaluators(event):
        		_post_any_code_evaluators = self._post_any_code_evaluators
        		if len(_post_any_code_evaluators) > 0:
        			for id, evaluator in _post_any_code_evaluators:
        				try:
        					evaluator()
        				except Exception, e:
        					print "Exception while running Any Code evaluator for '%s':" % (id), e
        			del  _post_any_code_evaluators[0:len(_post_any_code_evaluators)]
        	self._run_evaluators = _run_evaluators
        	try:
        		self.GetWin().Connect(-1, -1, wxEVT_AnyCode, _run_evaluators)
        	except:
        		pass	# FIXME
        	def _run_evaluators_later(evaluator=None):
        		if evaluator is not None:
        			self._post_any_code_evaluators += [evaluator]
        		try:
        			de = wx.PyEvent()
        			de.SetEventType(wxEVT_AnyCode)
        			wx.PostEvent(self.GetWin(), de)
        		except TypeError:
        			pass
        		except AttributeError:	# FIXME
        			print "Cannot post message"
        	self._run_evaluators_later = _run_evaluators_later
        	_run_evaluators_later()
        def __post_evalutate_gain_default():
        	self._post_any_code_evaluators += [('gain_default', lambda: self._evalutate_gain_default(**{'initial_gain': initial_gain}))]
        def __evalutate_gain_default(*args, **kwds):
        	try:
        		self.gain_default = [gain,initial_gain][gain==float('-inf')]
        		self.set_gain_default(self.gain_default)
        	except AttributeError, e:
        		print "AttributeError while evaulating gain_default:", e
        		__post_evalutate_gain_default()
        	except Exception, e:
        		print "Exception while evaluating gain_default:", e
        self._evalutate_gain_default = __evalutate_gain_default
        self.__post_evalutate_gain_default = __post_evalutate_gain_default
        self._evalutate_gain_default(**{'initial_gain': initial_gain})
        gain_default = self.gain_default
        _freq_fine_sizer = wx.BoxSizer(wx.VERTICAL)
        self._freq_fine_text_box = forms.text_box(
        	parent=self.nb_freq.GetPage(0).GetWin(),
        	sizer=_freq_fine_sizer,
        	value=self.freq_fine,
        	callback=self.set_freq_fine,
        	label="Freq (fine)",
        	converter=forms.float_converter(),
        	proportion=0,
        )
        self._freq_fine_slider = forms.slider(
        	parent=self.nb_freq.GetPage(0).GetWin(),
        	sizer=_freq_fine_sizer,
        	value=self.freq_fine,
        	callback=self.set_freq_fine,
        	minimum=-freq_fine_range/2,
        	maximum=freq_fine_range/2,
        	num_steps=1000,
        	style=wx.SL_HORIZONTAL,
        	cast=float,
        	proportion=1,
        )
        self.nb_freq.GetPage(0).GridAdd(_freq_fine_sizer, 2, 0, 1, 1)
        if not hasattr(self, '_post_any_code_evaluators'):
        	self._post_any_code_evaluators = []
        	self.wxEVT_AnyCode = wxEVT_AnyCode = wx.NewEventType()
        	def _run_evaluators(event):
        		_post_any_code_evaluators = self._post_any_code_evaluators
        		if len(_post_any_code_evaluators) > 0:
        			for id, evaluator in _post_any_code_evaluators:
        				try:
        					evaluator()
        				except Exception, e:
        					print "Exception while running Any Code evaluator for '%s':" % (id), e
        			del  _post_any_code_evaluators[0:len(_post_any_code_evaluators)]
        	self._run_evaluators = _run_evaluators
        	try:
        		self.GetWin().Connect(-1, -1, wxEVT_AnyCode, _run_evaluators)
        	except:
        		pass	# FIXME
        	def _run_evaluators_later(evaluator=None):
        		if evaluator is not None:
        			self._post_any_code_evaluators += [evaluator]
        		try:
        			de = wx.PyEvent()
        			de.SetEventType(wxEVT_AnyCode)
        			wx.PostEvent(self.GetWin(), de)
        		except TypeError:
        			pass
        		except AttributeError:	# FIXME
        			print "Cannot post message"
        	self._run_evaluators_later = _run_evaluators_later
        	_run_evaluators_later()
        def __post_evalutate_fix_invalid_freq():
        	self._post_any_code_evaluators += [('fix_invalid_freq', lambda: self._evalutate_fix_invalid_freq(**{}))]
        def __evalutate_fix_invalid_freq(*args, **kwds):
        	try:
        		((freq <= freq_range.stop()) and (freq >= freq_range.start())) or self.set_requested_freq_txt(self.src.get_center_freq()); print self.src.get_center_freq()
        		self.set_fix_invalid_freq(self.fix_invalid_freq)
        	except AttributeError, e:
        		print "AttributeError while evaulating fix_invalid_freq:", e
        		__post_evalutate_fix_invalid_freq()
        	except Exception, e:
        		print "Exception while evaluating fix_invalid_freq:", e
        self._evalutate_fix_invalid_freq = __evalutate_fix_invalid_freq
        self.__post_evalutate_fix_invalid_freq = __post_evalutate_fix_invalid_freq
        self.__post_evalutate_fix_invalid_freq()
        self.fft_sink_two_1 = fftsink2.fft_sink_c(
        	self.nb.GetPage(0).GetWin(),
        	baseband_freq=fft_center_freq,
        	y_per_div=10,
        	y_divs=int(dyn_rng/10),
        	ref_level=ref_lvl,
        	ref_scale=fft_ref_scale,
        	sample_rate=samp_rate,
        	fft_size=1024*0 + fft_size,
        	fft_rate=fft_rate,
        	average=True,
        	avg_alpha=ave,
        	title="FFT Plot 2",
        	peak_hold=False,	fft_in=False,
        	always_run=False,
        	fft_out=False,
        )
        self.nb.GetPage(0).Add(self.fft_sink_two_1.win)
        def fft_sink_two_1_callback(x, y):
        	self.set_clicked_freq(x)

        self.fft_sink_two_1.set_callback(fft_sink_two_1_callback)
        self.fft_max_lvl_probe = blocks.probe_signal_f()
        if probe_interval > 0:
            self._fft_max_lvl_2_poll_rate = probe_interval
        else:
            self._fft_max_lvl_2_poll_rate = 1
        self._fft_max_lvl_2_enabled = probe_interval > 0
        def _set_fft_max_lvl_2_poll_rate(rate):
            self._fft_max_lvl_2_enabled = rate > 0
            if rate > 0:
                self._fft_max_lvl_2_poll_rate = rate
        self.set_fft_max_lvl_2_poll_rate = _set_fft_max_lvl_2_poll_rate
        def _fft_max_lvl_2_probe():
            while True:
                if self._fft_max_lvl_2_enabled:
                    val = self.fft_max_lvl_probe_2.level()
                    try:
                        self.set_fft_max_lvl_2(val)
                    except AttributeError:
                        pass
                time.sleep(1.0/(self._fft_max_lvl_2_poll_rate))
        _fft_max_lvl_2_thread = threading.Thread(target=_fft_max_lvl_2_probe)
        _fft_max_lvl_2_thread.daemon = True
        _fft_max_lvl_2_thread.start()
        if probe_interval > 0:
            self._fft_max_lvl_poll_rate = probe_interval
        else:
            self._fft_max_lvl_poll_rate = 1
        self._fft_max_lvl_enabled = probe_interval > 0
        def _set_fft_max_lvl_poll_rate(rate):
            self._fft_max_lvl_enabled = rate > 0
            if rate > 0:
                self._fft_max_lvl_poll_rate = rate
        self.set_fft_max_lvl_poll_rate = _set_fft_max_lvl_poll_rate
        def _fft_max_lvl_probe():
            while True:
                if self._fft_max_lvl_enabled:
                    val = self.fft_max_lvl_probe.level()
                    try:
                        self.set_fft_max_lvl(val)
                    except AttributeError:
                        pass
                time.sleep(1.0/(self._fft_max_lvl_poll_rate))
        _fft_max_lvl_thread = threading.Thread(target=_fft_max_lvl_probe)
        _fft_max_lvl_thread.daemon = True
        _fft_max_lvl_thread.start()
        self.fft_max_idx_probe_0 = blocks.probe_signal_s()
        self.fft_max_idx_probe = blocks.probe_signal_s()
        if probe_interval > 0:
            self._fft_max_idx_poll_rate = probe_interval
        else:
            self._fft_max_idx_poll_rate = 1
        self._fft_max_idx_enabled = probe_interval > 0
        def _set_fft_max_idx_poll_rate(rate):
            self._fft_max_idx_enabled = rate > 0
            if rate > 0:
                self._fft_max_idx_poll_rate = rate
        self.set_fft_max_idx_poll_rate = _set_fft_max_idx_poll_rate
        def _fft_max_idx_probe():
            while True:
                if self._fft_max_idx_enabled:
                    val = self.fft_max_idx_probe.level()
                    try:
                        self.set_fft_max_idx(val)
                    except AttributeError:
                        pass
                time.sleep(1.0/(self._fft_max_idx_poll_rate))
        _fft_max_idx_thread = threading.Thread(target=_fft_max_idx_probe)
        _fft_max_idx_thread.daemon = True
        _fft_max_idx_thread.start()
        _decim_sizer = wx.BoxSizer(wx.VERTICAL)
        self._decim_text_box = forms.text_box(
        	parent=self.nb_rate.GetPage(0).GetWin(),
        	sizer=_decim_sizer,
        	value=self.decim,
        	callback=self.set_decim,
        	label="Divide base sample rate",
        	converter=forms.int_converter(),
        	proportion=0,
        )
        self._decim_slider = forms.slider(
        	parent=self.nb_rate.GetPage(0).GetWin(),
        	sizer=_decim_sizer,
        	value=self.decim,
        	callback=self.set_decim,
        	minimum=1,
        	maximum=max_decim,
        	num_steps=max_decim-1,
        	style=wx.SL_HORIZONTAL,
        	cast=int,
        	proportion=1,
        )
        self.nb_rate.GetPage(0).Add(_decim_sizer)
        if not hasattr(self, '_post_any_code_evaluators'):
        	self._post_any_code_evaluators = []
        	self.wxEVT_AnyCode = wxEVT_AnyCode = wx.NewEventType()
        	def _run_evaluators(event):
        		_post_any_code_evaluators = self._post_any_code_evaluators
        		if len(_post_any_code_evaluators) > 0:
        			for id, evaluator in _post_any_code_evaluators:
        				try:
        					evaluator()
        				except Exception, e:
        					print "Exception while running Any Code evaluator for '%s':" % (id), e
        			del  _post_any_code_evaluators[0:len(_post_any_code_evaluators)]
        	self._run_evaluators = _run_evaluators
        	try:
        		self.GetWin().Connect(-1, -1, wxEVT_AnyCode, _run_evaluators)
        	except:
        		pass	# FIXME
        	def _run_evaluators_later(evaluator=None):
        		if evaluator is not None:
        			self._post_any_code_evaluators += [evaluator]
        		try:
        			de = wx.PyEvent()
        			de.SetEventType(wxEVT_AnyCode)
        			wx.PostEvent(self.GetWin(), de)
        		except TypeError:
        			pass
        		except AttributeError:	# FIXME
        			print "Cannot post message"
        	self._run_evaluators_later = _run_evaluators_later
        	_run_evaluators_later()
        def __post_evalutate_daughterboard_sensors():
        	self._post_any_code_evaluators += [('daughterboard_sensors', lambda: self._evalutate_daughterboard_sensors(**{'daughterboard_sensor_names': daughterboard_sensor_names}))]
        def __evalutate_daughterboard_sensors(*args, **kwds):
        	try:
        		self.daughterboard_sensors = '\n'.join(map(lambda x: '%s: %s' % (x, str(self.src.get_sensor(x))), self.daughterboard_sensor_names))
        		self.set_daughterboard_sensors(self.daughterboard_sensors)
        	except AttributeError, e:
        		print "AttributeError while evaulating daughterboard_sensors:", e
        		__post_evalutate_daughterboard_sensors()
        	except Exception, e:
        		print "Exception while evaluating daughterboard_sensors:", e
        self._evalutate_daughterboard_sensors = __evalutate_daughterboard_sensors
        self.__post_evalutate_daughterboard_sensors = __post_evalutate_daughterboard_sensors
        self._evalutate_daughterboard_sensors(**{'daughterboard_sensor_names': daughterboard_sensor_names})
        daughterboard_sensors = self.daughterboard_sensors
        self._clicked_freq_txt_text_box = forms.text_box(
        	parent=self.nb_test.GetPage(0).GetWin(),
        	value=self.clicked_freq_txt,
        	callback=self.set_clicked_freq_txt,
        	label="Clicked freq",
        	converter=forms.float_converter(),
        )
        self.nb_test.GetPage(0).Add(self._clicked_freq_txt_text_box)
        self.blocks_tag_debug_0 = blocks.tag_debug(gr.sizeof_gr_complex*1, "", ""); self.blocks_tag_debug_0.set_display(show_stream_tags_chk)
        self.blocks_null_sink_0_0 = blocks.null_sink(gr.sizeof_short*1)
        self.blocks_null_sink_0 = blocks.null_sink(gr.sizeof_short*1)
        self.blocks_max_xx_0_0 = blocks.max_ff(fft_size)
        self.blocks_max_xx_0 = blocks.max_ff(fft_size)
        self.blocks_float_to_complex_0_0 = blocks.float_to_complex(1)
        self.blocks_float_to_complex_0 = blocks.float_to_complex(1)
        self.blocks_complex_to_mag_0_0 = blocks.complex_to_mag(1)
        self.blocks_complex_to_mag_0 = blocks.complex_to_mag(1)
        self.blocks_argmax_xx_0_0 = blocks.argmax_fs(fft_size)
        self.blocks_argmax_xx_0 = blocks.argmax_fs(fft_size)
        self.blks2_selector_0_1 = grc_blks2.selector(
        	item_size=gr.sizeof_gr_complex*1,
        	num_inputs=2,
        	num_outputs=1,
        	input_index=scope_mode_2,
        	output_index=0,
        )
        self.blks2_selector_0_0_0 = grc_blks2.selector(
        	item_size=gr.sizeof_gr_complex*1,
        	num_inputs=1,
        	num_outputs=2,
        	input_index=0,
        	output_index=scope_mode_2,
        )
        self.blks2_selector_0_0 = grc_blks2.selector(
        	item_size=gr.sizeof_gr_complex*1,
        	num_inputs=1,
        	num_outputs=2,
        	input_index=0,
        	output_index=scope_mode,
        )
        self.blks2_selector_0 = grc_blks2.selector(
        	item_size=gr.sizeof_gr_complex*1,
        	num_inputs=2,
        	num_outputs=1,
        	input_index=scope_mode,
        	output_index=0,
        )
        if not hasattr(self, '_post_any_code_evaluators'):
        	self._post_any_code_evaluators = []
        	self.wxEVT_AnyCode = wxEVT_AnyCode = wx.NewEventType()
        	def _run_evaluators(event):
        		_post_any_code_evaluators = self._post_any_code_evaluators
        		if len(_post_any_code_evaluators) > 0:
        			for id, evaluator in _post_any_code_evaluators:
        				try:
        					evaluator()
        				except Exception, e:
        					print "Exception while running Any Code evaluator for '%s':" % (id), e
        			del  _post_any_code_evaluators[0:len(_post_any_code_evaluators)]
        	self._run_evaluators = _run_evaluators
        	try:
        		self.GetWin().Connect(-1, -1, wxEVT_AnyCode, _run_evaluators)
        	except:
        		pass	# FIXME
        	def _run_evaluators_later(evaluator=None):
        		if evaluator is not None:
        			self._post_any_code_evaluators += [evaluator]
        		try:
        			de = wx.PyEvent()
        			de.SetEventType(wxEVT_AnyCode)
        			wx.PostEvent(self.GetWin(), de)
        		except TypeError:
        			pass
        		except AttributeError:	# FIXME
        			print "Cannot post message"
        	self._run_evaluators_later = _run_evaluators_later
        	_run_evaluators_later()
        def __post_evalutate_any_test_1():
        	self._post_any_code_evaluators += [('any_test_1', lambda: self._evalutate_any_test_1(**{'test': test}))]
        def __evalutate_any_test_1(*args, **kwds):
        	try:
        		if self.fft_sink.controller['average']: self.fft_sink.controller['average'] = False; time.sleep(0.25); self.fft_sink.controller['average'] = True;
        		self.set_any_test_1(self.any_test_1)
        	except AttributeError, e:
        		print "AttributeError while evaulating any_test_1:", e
        		__post_evalutate_any_test_1()
        	except Exception, e:
        		print "Exception while evaluating any_test_1:", e
        self._evalutate_any_test_1 = __evalutate_any_test_1
        self.__post_evalutate_any_test_1 = __post_evalutate_any_test_1
        if not hasattr(self, '_post_any_code_evaluators'):
        	self._post_any_code_evaluators = []
        	self.wxEVT_AnyCode = wxEVT_AnyCode = wx.NewEventType()
        	def _run_evaluators(event):
        		_post_any_code_evaluators = self._post_any_code_evaluators
        		if len(_post_any_code_evaluators) > 0:
        			for id, evaluator in _post_any_code_evaluators:
        				try:
        					evaluator()
        				except Exception, e:
        					print "Exception while running Any Code evaluator for '%s':" % (id), e
        			del  _post_any_code_evaluators[0:len(_post_any_code_evaluators)]
        	self._run_evaluators = _run_evaluators
        	try:
        		self.GetWin().Connect(-1, -1, wxEVT_AnyCode, _run_evaluators)
        	except:
        		pass	# FIXME
        	def _run_evaluators_later(evaluator=None):
        		if evaluator is not None:
        			self._post_any_code_evaluators += [evaluator]
        		try:
        			de = wx.PyEvent()
        			de.SetEventType(wxEVT_AnyCode)
        			wx.PostEvent(self.GetWin(), de)
        		except TypeError:
        			pass
        		except AttributeError:	# FIXME
        			print "Cannot post message"
        	self._run_evaluators_later = _run_evaluators_later
        	_run_evaluators_later()
        def __post_evalutate_actual_sample_rate():
        	self._post_any_code_evaluators += [('actual_sample_rate', lambda: self._evalutate_actual_sample_rate(**{'requested_sample_rate': requested_sample_rate}))]
        def __evalutate_actual_sample_rate(*args, **kwds):
        	try:
        		self.actual_sample_rate = self.src.get_samp_rate()
        		self.set_actual_sample_rate(self.actual_sample_rate)
        	except AttributeError, e:
        		print "AttributeError while evaulating actual_sample_rate:", e
        		__post_evalutate_actual_sample_rate()
        	except Exception, e:
        		print "Exception while evaluating actual_sample_rate:", e
        self._evalutate_actual_sample_rate = __evalutate_actual_sample_rate
        self.__post_evalutate_actual_sample_rate = __post_evalutate_actual_sample_rate
        self._evalutate_actual_sample_rate(**{'requested_sample_rate': requested_sample_rate})
        actual_sample_rate = self.actual_sample_rate
        if not hasattr(self, '_post_any_code_evaluators'):
        	self._post_any_code_evaluators = []
        	self.wxEVT_AnyCode = wxEVT_AnyCode = wx.NewEventType()
        	def _run_evaluators(event):
        		_post_any_code_evaluators = self._post_any_code_evaluators
        		if len(_post_any_code_evaluators) > 0:
        			for id, evaluator in _post_any_code_evaluators:
        				try:
        					evaluator()
        				except Exception, e:
        					print "Exception while running Any Code evaluator for '%s':" % (id), e
        			del  _post_any_code_evaluators[0:len(_post_any_code_evaluators)]
        	self._run_evaluators = _run_evaluators
        	try:
        		self.GetWin().Connect(-1, -1, wxEVT_AnyCode, _run_evaluators)
        	except:
        		pass	# FIXME
        	def _run_evaluators_later(evaluator=None):
        		if evaluator is not None:
        			self._post_any_code_evaluators += [evaluator]
        		try:
        			de = wx.PyEvent()
        			de.SetEventType(wxEVT_AnyCode)
        			wx.PostEvent(self.GetWin(), de)
        		except TypeError:
        			pass
        		except AttributeError:	# FIXME
        			print "Cannot post message"
        	self._run_evaluators_later = _run_evaluators_later
        	_run_evaluators_later()
        def __post_evalutate_actual_center_freq():
        	self._post_any_code_evaluators += [('actual_center_freq', lambda: self._evalutate_actual_center_freq(**{'tune_obj': tune_obj}))]
        def __evalutate_actual_center_freq(*args, **kwds):
        	try:
        		self.actual_center_freq = self.src.get_center_freq()
        		self.set_actual_center_freq(self.actual_center_freq)
        	except AttributeError, e:
        		print "AttributeError while evaulating actual_center_freq:", e
        		__post_evalutate_actual_center_freq()
        	except Exception, e:
        		print "Exception while evaluating actual_center_freq:", e
        self._evalutate_actual_center_freq = __evalutate_actual_center_freq
        self.__post_evalutate_actual_center_freq = __post_evalutate_actual_center_freq
        self._evalutate_actual_center_freq(**{'tune_obj': tune_obj})
        actual_center_freq = self.actual_center_freq

        ##################################################
        # Connections
        ##################################################
        self.connect((self.blks2_selector_0, 0), (self.wxgui_scopesink2_0, 0))
        self.connect((self.blks2_selector_0_0, 0), (self.blks2_selector_0, 0))
        self.connect((self.blks2_selector_0_0, 1), (self.blocks_complex_to_mag_0, 0))
        self.connect((self.blks2_selector_0_0_0, 0), (self.blks2_selector_0_1, 0))
        self.connect((self.blks2_selector_0_0_0, 1), (self.blocks_complex_to_mag_0_0, 0))
        self.connect((self.blks2_selector_0_1, 0), (self.wxgui_scopesink2_0_0, 0))
        self.connect((self.blocks_argmax_xx_0, 1), (self.blocks_null_sink_0, 0))
        self.connect((self.blocks_argmax_xx_0, 0), (self.fft_max_idx_probe, 0))
        self.connect((self.blocks_argmax_xx_0_0, 1), (self.blocks_null_sink_0_0, 0))
        self.connect((self.blocks_argmax_xx_0_0, 0), (self.fft_max_idx_probe_0, 0))
        self.connect((self.blocks_complex_to_mag_0, 0), (self.blocks_float_to_complex_0, 1))
        self.connect((self.blocks_complex_to_mag_0, 0), (self.blocks_float_to_complex_0, 0))
        self.connect((self.blocks_complex_to_mag_0_0, 0), (self.blocks_float_to_complex_0_0, 1))
        self.connect((self.blocks_complex_to_mag_0_0, 0), (self.blocks_float_to_complex_0_0, 0))
        self.connect((self.blocks_float_to_complex_0, 0), (self.blks2_selector_0, 1))
        self.connect((self.blocks_float_to_complex_0_0, 0), (self.blks2_selector_0_1, 1))
        self.connect((self.blocks_max_xx_0, 0), (self.fft_max_lvl_probe, 0))
        self.connect((self.blocks_max_xx_0_0, 0), (self.fft_max_lvl_probe_2, 0))
        self.connect((self.logpwrfft_x_0, 0), (self.blocks_argmax_xx_0, 0))
        self.connect((self.logpwrfft_x_0, 0), (self.blocks_max_xx_0, 0))
        self.connect((self.logpwrfft_x_0_0, 0), (self.blocks_argmax_xx_0_0, 0))
        self.connect((self.logpwrfft_x_0_0, 0), (self.blocks_max_xx_0_0, 0))
        self.connect((self.src, 0), (self.blocks_tag_debug_0, 0))
        self.connect((self.src, 0), (self.blks2_selector_0_0, 0))
        self.connect((self.src, 0), (self.fft_sink, 0))
        self.connect((self.src, 0), (self.waterfall_sink, 0))
        self.connect((self.src, 0), (self.probe_avg_mag, 0))
        self.connect((self.src, 0), (self.logpwrfft_x_0, 0))
        self.connect((self.src, 1), (self.blks2_selector_0_0_0, 0))
        self.connect((self.src, 1), (self.fft_sink_two_1, 0))
        self.connect((self.src, 1), (self.logpwrfft_x_0_0, 0))
        self.connect((self.src, 1), (self.waterfall_sink_0, 0))


    def get_antenna(self):
        return self.antenna

    def set_antenna(self, antenna):
        self.antenna = antenna
        self.set_selected_antenna([ [self.antenna,self.antennas[0]][self.antenna not in self.antennas] ,self.antennas[0]][self.antenna==''])

    def get_args(self):
        return self.args

    def set_args(self, args):
        self.args = args

    def get_ave(self):
        return self.ave

    def set_ave(self, ave):
        self.ave = ave
        self.set_fft_ave_probe(self.ave)

    def get_averaging(self):
        return self.averaging

    def set_averaging(self, averaging):
        self.averaging = averaging
        self.set_fft_averaging(self.str_to_bool(self.averaging))

    def get_bw(self):
        return self.bw

    def set_bw(self, bw):
        self.bw = bw
        self.src.set_bandwidth(self.bw, 0)
        self.src.set_bandwidth(self.bw, 1)

    def get_dyn_rng(self):
        return self.dyn_rng

    def set_dyn_rng(self, dyn_rng):
        self.dyn_rng = dyn_rng

    def get_fft_rate(self):
        return self.fft_rate

    def set_fft_rate(self, fft_rate):
        self.fft_rate = fft_rate

    def get_fft_ref_scale(self):
        return self.fft_ref_scale

    def set_fft_ref_scale(self, fft_ref_scale):
        self.fft_ref_scale = fft_ref_scale

    def get_fft_size(self):
        return self.fft_size

    def set_fft_size(self, fft_size):
        self.fft_size = fft_size
        self.set_fft_max_idx((self.fft_size/2)*0)
        self.set_fft_max_idx_norm([self.fft_max_idx,self.fft_max_idx-self.fft_size][int(self.fft_max_idx>(self.fft_size/2))])
        self.set_max_bin_freq(self.fft_center_freq + (((1.0*self.fft_max_idx_norm) / self.fft_size) * self.samp_rate))

    def get_freq(self):
        return self.freq

    def set_freq(self, freq):
        self.freq = freq
        self.set_requested_freq_txt(self.freq)
        self.src.set_center_freq(uhd.tune_request(self.freq, self.lo_offset), 0)
        self.src.set_center_freq(uhd.tune_request(self.freq, self.lo_offset), 1)

    def get_freq_fine_range(self):
        return self.freq_fine_range

    def set_freq_fine_range(self, freq_fine_range):
        self.freq_fine_range = freq_fine_range

    def get_gain(self):
        return self.gain

    def set_gain(self, gain):
        self.gain = gain

    def get_lo_check_interval(self):
        return self.lo_check_interval

    def set_lo_check_interval(self, lo_check_interval):
        self.lo_check_interval = lo_check_interval
        self.set_locked_probe_poll_rate(self.lo_check_interval * float(self.has_lo_locked))

    def get_lo_offset(self):
        return self.lo_offset

    def set_lo_offset(self, lo_offset):
        self.lo_offset = lo_offset
        self.set_lo_offset_txt(self.lo_offset)
        self.set_tune_mode([1,0][self.lo_offset==0.0])
        self.src.set_center_freq(uhd.tune_request(self.freq, self.lo_offset), 0)
        self.src.set_center_freq(uhd.tune_request(self.freq, self.lo_offset), 1)

    def get_mag_alpha(self):
        return self.mag_alpha

    def set_mag_alpha(self, mag_alpha):
        self.mag_alpha = mag_alpha
        self.probe_avg_mag.set_alpha(self.mag_alpha)

    def get_peak_hold(self):
        return self.peak_hold

    def set_peak_hold(self, peak_hold):
        self.peak_hold = peak_hold
        self.set_fft_peak_hold(self.str_to_bool(self.peak_hold))

    def get_pps(self):
        return self.pps

    def set_pps(self, pps):
        self.pps = pps
        self._evalutate_update_time_source(**{'pps': self.pps})

    def get_probe_interval(self):
        return self.probe_interval

    def set_probe_interval(self, probe_interval):
        self.probe_interval = probe_interval
        self.set_fft_max_idx_poll_rate(self.probe_interval)
        self.set_fft_max_lvl_poll_rate(self.probe_interval)
        self.set_fft_max_lvl_2_poll_rate(self.probe_interval)
        self.set_signal_probe_poll_rate(self.probe_interval)

    def get_rate(self):
        return self.rate

    def set_rate(self, rate):
        self.rate = rate
        self.set_requested_sample_rate_base(self.rate)

    def get_ref(self):
        return self.ref

    def set_ref(self, ref):
        self.ref = ref
        self._evalutate_update_clock_source(**{'ref': self.ref})

    def get_ref_lvl(self):
        return self.ref_lvl

    def set_ref_lvl(self, ref_lvl):
        self.ref_lvl = ref_lvl

    def get_sensor_interval(self):
        return self.sensor_interval

    def set_sensor_interval(self, sensor_interval):
        self.sensor_interval = sensor_interval
        self.set_daughterboard_sensor_names_poll_rate(self.sensor_interval)
        self.set_motherboard_sensor_names_poll_rate(self.sensor_interval)
        self.set_time_probe_poll_rate(self.sensor_interval)

    def get_show_stream_tags(self):
        return self.show_stream_tags

    def set_show_stream_tags(self, show_stream_tags):
        self.show_stream_tags = show_stream_tags
        self.set_show_stream_tags_chk([False, True][self.show_stream_tags.lower() != 'false'])

    def get_spec(self):
        return self.spec

    def set_spec(self, spec):
        self.spec = spec

    def get_stream_args(self):
        return self.stream_args

    def set_stream_args(self, stream_args):
        self.stream_args = stream_args

    def get_window(self):
        return self.window

    def set_window(self, window):
        self.window = window
        self.set_variable_static_fft_window_name(self.window_fn_name_map[self.window])
        self.set_window_fn(self.window_fn_map[self.window])

    def get_wire_format(self):
        return self.wire_format

    def set_wire_format(self, wire_format):
        self.wire_format = wire_format

    def get_relative_freq(self):
        return self.relative_freq

    def set_relative_freq(self, relative_freq):
        self.relative_freq = relative_freq
        self.set_fft_center_freq(self.actual_center_freq * self.relative_freq)
        self._relative_freq_chooser.set_value(self.relative_freq)

    def get_gain_range(self):
        return self.gain_range

    def set_gain_range(self, gain_range):
        self.gain_range = gain_range
        self._run_evaluators_later(('initial_gain', lambda: self._evalutate_initial_gain(**{'gain_range': self.gain_range})))

    def get_fft_max_idx(self):
        return self.fft_max_idx

    def set_fft_max_idx(self, fft_max_idx):
        self.fft_max_idx = fft_max_idx
        self.set_fft_max_idx_norm([self.fft_max_idx,self.fft_max_idx-self.fft_size][int(self.fft_max_idx>(self.fft_size/2))])

    def get_actual_sample_rate(self):
        return self.actual_sample_rate

    def set_actual_sample_rate(self, actual_sample_rate):
        self.actual_sample_rate = actual_sample_rate
        self.set_samp_rate(int(self.actual_sample_rate))
        self.set_variable_static_actual_sample_rate(self.actual_sample_rate)

    def get_actual_center_freq(self):
        return self.actual_center_freq

    def set_actual_center_freq(self, actual_center_freq):
        self.actual_center_freq = actual_center_freq
        self.set_fft_center_freq(self.actual_center_freq * self.relative_freq)

    def get_str_to_bool(self):
        return self.str_to_bool

    def set_str_to_bool(self, str_to_bool):
        self.str_to_bool = str_to_bool
        self.set_fft_averaging(self.str_to_bool(self.averaging))
        self.set_fft_peak_hold(self.str_to_bool(self.peak_hold))

    def get_signal_probe(self):
        return self.signal_probe

    def set_signal_probe(self, signal_probe):
        self.signal_probe = signal_probe
        self.set_signal_probe_log(math.log10([self.signal_probe,1.0][self.signal_probe==0.0])*10)

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.set_max_bin_freq(self.fft_center_freq + (((1.0*self.fft_max_idx_norm) / self.fft_size) * self.samp_rate))
        self.fft_sink.set_sample_rate(self.samp_rate)
        self.fft_sink_two_1.set_sample_rate(self.samp_rate)
        self.logpwrfft_x_0.set_sample_rate(self.samp_rate)
        self.logpwrfft_x_0_0.set_sample_rate(self.samp_rate)
        self.waterfall_sink.set_sample_rate(self.samp_rate)
        self.waterfall_sink_0.set_sample_rate(self.samp_rate)
        self.wxgui_scopesink2_0.set_sample_rate(self.samp_rate)
        self.wxgui_scopesink2_0_0.set_sample_rate(self.samp_rate)

    def get_requested_freq_txt(self):
        return self.requested_freq_txt

    def set_requested_freq_txt(self, requested_freq_txt):
        self.requested_freq_txt = requested_freq_txt
        self.set_requested_freq(self.requested_freq_txt + self.freq_fine)
        self._requested_freq_txt_slider.set_value(self.requested_freq_txt)
        self._requested_freq_txt_text_box.set_value(self.requested_freq_txt)

    def get_gain_default(self):
        return self.gain_default

    def set_gain_default(self, gain_default):
        self.gain_default = gain_default
        self.set_selected_gain(self.gain_default)

    def get_freq_fine(self):
        return self.freq_fine

    def set_freq_fine(self, freq_fine):
        self.freq_fine = freq_fine
        self._freq_fine_slider.set_value(self.freq_fine)
        self._freq_fine_text_box.set_value(self.freq_fine)
        self.set_requested_freq(self.requested_freq_txt + self.freq_fine)

    def get_fft_max_idx_norm(self):
        return self.fft_max_idx_norm

    def set_fft_max_idx_norm(self, fft_max_idx_norm):
        self.fft_max_idx_norm = fft_max_idx_norm
        self.set_max_bin_freq(self.fft_center_freq + (((1.0*self.fft_max_idx_norm) / self.fft_size) * self.samp_rate))

    def get_fft_center_freq(self):
        return self.fft_center_freq

    def set_fft_center_freq(self, fft_center_freq):
        self.fft_center_freq = fft_center_freq
        self.set_max_bin_freq(self.fft_center_freq + (((1.0*self.fft_max_idx_norm) / self.fft_size) * self.samp_rate))
        self.fft_sink.set_baseband_freq(self.fft_center_freq)
        self.fft_sink_two_1.set_baseband_freq(self.fft_center_freq)
        self.waterfall_sink.set_baseband_freq(self.fft_center_freq)
        self.waterfall_sink_0.set_baseband_freq(self.fft_center_freq)

    def get_antennas(self):
        return self.antennas

    def set_antennas(self, antennas):
        self.antennas = antennas
        self.set_selected_antenna([ [self.antenna,self.antennas[0]][self.antenna not in self.antennas] ,self.antennas[0]][self.antenna==''])

    def get_window_fn_name_map(self):
        return self.window_fn_name_map

    def set_window_fn_name_map(self, window_fn_name_map):
        self.window_fn_name_map = window_fn_name_map
        self.set_variable_static_fft_window_name(self.window_fn_name_map[self.window])

    def get_window_fn_map(self):
        return self.window_fn_map

    def set_window_fn_map(self, window_fn_map):
        self.window_fn_map = window_fn_map
        self.set_window_fn(self.window_fn_map[self.window])

    def get_usrp_info(self):
        return self.usrp_info

    def set_usrp_info(self, usrp_info):
        self.usrp_info = usrp_info
        self.set_variable_static_usrp_info(self.usrp_info)

    def get_tune_result(self):
        return self.tune_result

    def set_tune_result(self, tune_result):
        self.tune_result = tune_result
        self.set_variable_static_actual_dsp_freq(self.tune_result.actual_dsp_freq)
        self.set_variable_static_rf_freq(self.tune_result.actual_rf_freq)

    def get_tune_mode(self):
        return self.tune_mode

    def set_tune_mode(self, tune_mode):
        self.tune_mode = tune_mode
        self._tune_mode_chooser.set_value(self.tune_mode)
        self.set_tune_obj([self.requested_freq, uhd.tune_request(self.requested_freq, self.lo_offset_txt), uhd.tune_request(self.requested_freq, dsp_freq=0, dsp_freq_policy=uhd.tune_request.POLICY_MANUAL)][self.tune_mode])

    def get_time_probe(self):
        return self.time_probe

    def set_time_probe(self, time_probe):
        self.time_probe = time_probe
        self.set_variable_static_time_now(str( [time.ctime(self.time_probe.get_real_secs()), datetime.timedelta(seconds=self.time_probe.get_real_secs()), time.gmtime(self.time_probe.get_real_secs())] [self.relative_time]))

    def get_test(self):
        return self.test

    def set_test(self, test):
        self.test = test
        self._run_evaluators_later(('any_test_1', lambda: self._evalutate_any_test_1(**{'test': self.test})))
        self._test_chooser.set_value(self.test)

    def get_subdev_spec(self):
        return self.subdev_spec

    def set_subdev_spec(self, subdev_spec):
        self.subdev_spec = subdev_spec
        self.set_variable_static_subdev_spec(self.subdev_spec)

    def get_signal_probe_log(self):
        return self.signal_probe_log

    def set_signal_probe_log(self, signal_probe_log):
        self.signal_probe_log = signal_probe_log
        self.set_variable_static_level_probe(self.signal_probe_log)

    def get_selected_gain(self):
        return self.selected_gain

    def set_selected_gain(self, selected_gain):
        self.selected_gain = selected_gain
        self._selected_gain_slider.set_value(self.selected_gain)
        self._selected_gain_text_box.set_value(self.selected_gain)
        self.set_selected_gain_proxy(self.selected_gain)

    def get_selected_antenna(self):
        return self.selected_antenna

    def set_selected_antenna(self, selected_antenna):
        self.selected_antenna = selected_antenna
        self._selected_antenna_chooser.set_value(self.selected_antenna)
        self.set_selected_antenna_proxy(self.selected_antenna)

    def get_requested_sample_rate_base(self):
        return self.requested_sample_rate_base

    def set_requested_sample_rate_base(self, requested_sample_rate_base):
        self.requested_sample_rate_base = requested_sample_rate_base
        self.set_requested_sample_rate(self.requested_sample_rate_base / (1.*self.decim))
        self._requested_sample_rate_base_text_box.set_value(self.requested_sample_rate_base)

    def get_requested_freq(self):
        return self.requested_freq

    def set_requested_freq(self, requested_freq):
        self.requested_freq = requested_freq
        self.set_tune_obj([self.requested_freq, uhd.tune_request(self.requested_freq, self.lo_offset_txt), uhd.tune_request(self.requested_freq, dsp_freq=0, dsp_freq_policy=uhd.tune_request.POLICY_MANUAL)][self.tune_mode])
        self.set_variable_static_requested_freq(self.requested_freq)

    def get_relative_time(self):
        return self.relative_time

    def set_relative_time(self, relative_time):
        self.relative_time = relative_time
        self._relative_time_chooser.set_value(self.relative_time)
        self.set_variable_static_time_now(str( [time.ctime(self.time_probe.get_real_secs()), datetime.timedelta(seconds=self.time_probe.get_real_secs()), time.gmtime(self.time_probe.get_real_secs())] [self.relative_time]))

    def get_motherboard_sensors(self):
        return self.motherboard_sensors

    def set_motherboard_sensors(self, motherboard_sensors):
        self.motherboard_sensors = motherboard_sensors
        self.set_variable_static_text_0(self.motherboard_sensors)

    def get_max_bin_freq(self):
        return self.max_bin_freq

    def set_max_bin_freq(self, max_bin_freq):
        self.max_bin_freq = max_bin_freq
        self.set_variable_static_max_bin_freq(self.max_bin_freq)

    def get_locked_probe(self):
        return self.locked_probe

    def set_locked_probe(self, locked_probe):
        self.locked_probe = locked_probe
        self.set_static_locked('"' + str(self.locked_probe) + '"')

    def get_lo_offset_txt(self):
        return self.lo_offset_txt

    def set_lo_offset_txt(self, lo_offset_txt):
        self.lo_offset_txt = lo_offset_txt
        self._lo_offset_txt_text_box.set_value(self.lo_offset_txt)
        self.set_tune_obj([self.requested_freq, uhd.tune_request(self.requested_freq, self.lo_offset_txt), uhd.tune_request(self.requested_freq, dsp_freq=0, dsp_freq_policy=uhd.tune_request.POLICY_MANUAL)][self.tune_mode])

    def get_fft_peak_hold(self):
        return self.fft_peak_hold

    def set_fft_peak_hold(self, fft_peak_hold):
        self.fft_peak_hold = fft_peak_hold
        self._run_evaluators_later(('variable_any_code_fft_sink_peak_hold', lambda: self._evalutate_variable_any_code_fft_sink_peak_hold(**{'peak_hold': self.fft_peak_hold})))

    def get_fft_max_lvl_2(self):
        return self.fft_max_lvl_2

    def set_fft_max_lvl_2(self, fft_max_lvl_2):
        self.fft_max_lvl_2 = fft_max_lvl_2
        self.set_fft_max_lvl_value_2(self.fft_max_lvl_2)

    def get_fft_max_lvl(self):
        return self.fft_max_lvl

    def set_fft_max_lvl(self, fft_max_lvl):
        self.fft_max_lvl = fft_max_lvl
        self.set_fft_max_lvl_value(self.fft_max_lvl)
        self.set_variable_static_fft_max_lvl_log(self.fft_max_lvl)

    def get_fft_averaging(self):
        return self.fft_averaging

    def set_fft_averaging(self, fft_averaging):
        self.fft_averaging = fft_averaging
        self._run_evaluators_later(('variable_any_code_fft_sink_ave', lambda: self._evalutate_variable_any_code_fft_sink_ave(**{'averaging': self.fft_averaging})))
        self._run_evaluators_later(('variable_any_code_waterfall_ave', lambda: self._evalutate_variable_any_code_waterfall_ave(**{'averaging': self.fft_averaging})))

    def get_decim(self):
        return self.decim

    def set_decim(self, decim):
        self.decim = decim
        self._decim_slider.set_value(self.decim)
        self._decim_text_box.set_value(self.decim)
        self.set_requested_sample_rate(self.requested_sample_rate_base / (1.*self.decim))

    def get_daughterboard_sensors(self):
        return self.daughterboard_sensors

    def set_daughterboard_sensors(self, daughterboard_sensors):
        self.daughterboard_sensors = daughterboard_sensors
        self.set_variable_static_text_0_0(self.daughterboard_sensors)

    def get_clicked_freq(self):
        return self.clicked_freq

    def set_clicked_freq(self, clicked_freq):
        self.clicked_freq = clicked_freq
        self.set_clicked_freq_txt(self.clicked_freq)

    def get_window_fn(self):
        return self.window_fn

    def set_window_fn(self, window_fn):
        self.window_fn = window_fn

    def get_variable_static_usrp_info(self):
        return self.variable_static_usrp_info

    def set_variable_static_usrp_info(self, variable_static_usrp_info):
        self.variable_static_usrp_info = variable_static_usrp_info
        self._variable_static_usrp_info_static_text.set_value(self.variable_static_usrp_info)

    def get_variable_static_time_now(self):
        return self.variable_static_time_now

    def set_variable_static_time_now(self, variable_static_time_now):
        self.variable_static_time_now = variable_static_time_now
        self._variable_static_time_now_static_text.set_value(self.variable_static_time_now)

    def get_variable_static_text_0_0(self):
        return self.variable_static_text_0_0

    def set_variable_static_text_0_0(self, variable_static_text_0_0):
        self.variable_static_text_0_0 = variable_static_text_0_0
        self._variable_static_text_0_0_static_text.set_value(self.variable_static_text_0_0)

    def get_variable_static_text_0(self):
        return self.variable_static_text_0

    def set_variable_static_text_0(self, variable_static_text_0):
        self.variable_static_text_0 = variable_static_text_0
        self._variable_static_text_0_static_text.set_value(self.variable_static_text_0)

    def get_variable_static_subdev_spec(self):
        return self.variable_static_subdev_spec

    def set_variable_static_subdev_spec(self, variable_static_subdev_spec):
        self.variable_static_subdev_spec = variable_static_subdev_spec
        self._variable_static_subdev_spec_static_text.set_value(self.variable_static_subdev_spec)

    def get_variable_static_rf_freq(self):
        return self.variable_static_rf_freq

    def set_variable_static_rf_freq(self, variable_static_rf_freq):
        self.variable_static_rf_freq = variable_static_rf_freq
        self._variable_static_rf_freq_static_text.set_value(self.variable_static_rf_freq)

    def get_variable_static_requested_freq(self):
        return self.variable_static_requested_freq

    def set_variable_static_requested_freq(self, variable_static_requested_freq):
        self.variable_static_requested_freq = variable_static_requested_freq
        self._variable_static_requested_freq_static_text.set_value(self.variable_static_requested_freq)

    def get_variable_static_max_bin_freq(self):
        return self.variable_static_max_bin_freq

    def set_variable_static_max_bin_freq(self, variable_static_max_bin_freq):
        self.variable_static_max_bin_freq = variable_static_max_bin_freq
        self._variable_static_max_bin_freq_static_text.set_value(self.variable_static_max_bin_freq)

    def get_variable_static_level_probe(self):
        return self.variable_static_level_probe

    def set_variable_static_level_probe(self, variable_static_level_probe):
        self.variable_static_level_probe = variable_static_level_probe
        self._variable_static_level_probe_static_text.set_value(self.variable_static_level_probe)

    def get_variable_static_fft_window_name(self):
        return self.variable_static_fft_window_name

    def set_variable_static_fft_window_name(self, variable_static_fft_window_name):
        self.variable_static_fft_window_name = variable_static_fft_window_name
        self._variable_static_fft_window_name_static_text.set_value(self.variable_static_fft_window_name)

    def get_variable_static_fft_max_lvl_log(self):
        return self.variable_static_fft_max_lvl_log

    def set_variable_static_fft_max_lvl_log(self, variable_static_fft_max_lvl_log):
        self.variable_static_fft_max_lvl_log = variable_static_fft_max_lvl_log
        self._variable_static_fft_max_lvl_log_static_text.set_value(self.variable_static_fft_max_lvl_log)

    def get_variable_static_actual_sample_rate(self):
        return self.variable_static_actual_sample_rate

    def set_variable_static_actual_sample_rate(self, variable_static_actual_sample_rate):
        self.variable_static_actual_sample_rate = variable_static_actual_sample_rate
        self._variable_static_actual_sample_rate_static_text.set_value(self.variable_static_actual_sample_rate)

    def get_variable_static_actual_dsp_freq(self):
        return self.variable_static_actual_dsp_freq

    def set_variable_static_actual_dsp_freq(self, variable_static_actual_dsp_freq):
        self.variable_static_actual_dsp_freq = variable_static_actual_dsp_freq
        self._variable_static_actual_dsp_freq_static_text.set_value(self.variable_static_actual_dsp_freq)

    def get_variable_any_code_waterfall_ave(self):
        return self.variable_any_code_waterfall_ave

    def set_variable_any_code_waterfall_ave(self, variable_any_code_waterfall_ave):
        self.variable_any_code_waterfall_ave = variable_any_code_waterfall_ave

    def get_variable_any_code_iq_correction(self):
        return self.variable_any_code_iq_correction

    def set_variable_any_code_iq_correction(self, variable_any_code_iq_correction):
        self.variable_any_code_iq_correction = variable_any_code_iq_correction

    def get_variable_any_code_fft_sink_peak_hold(self):
        return self.variable_any_code_fft_sink_peak_hold

    def set_variable_any_code_fft_sink_peak_hold(self, variable_any_code_fft_sink_peak_hold):
        self.variable_any_code_fft_sink_peak_hold = variable_any_code_fft_sink_peak_hold

    def get_variable_any_code_fft_sink_ave(self):
        return self.variable_any_code_fft_sink_ave

    def set_variable_any_code_fft_sink_ave(self, variable_any_code_fft_sink_ave):
        self.variable_any_code_fft_sink_ave = variable_any_code_fft_sink_ave

    def get_variable_any_code_auto_dc_offset_removal(self):
        return self.variable_any_code_auto_dc_offset_removal

    def set_variable_any_code_auto_dc_offset_removal(self, variable_any_code_auto_dc_offset_removal):
        self.variable_any_code_auto_dc_offset_removal = variable_any_code_auto_dc_offset_removal

    def get_update_time_source(self):
        return self.update_time_source

    def set_update_time_source(self, update_time_source):
        self.update_time_source = update_time_source

    def get_update_clock_source(self):
        return self.update_clock_source

    def set_update_clock_source(self, update_clock_source):
        self.update_clock_source = update_clock_source

    def get_tune_obj(self):
        return self.tune_obj

    def set_tune_obj(self, tune_obj):
        self.tune_obj = tune_obj
        self._run_evaluators_later(('actual_center_freq', lambda: self._evalutate_actual_center_freq(**{'tune_obj': self.tune_obj})))
        self._run_evaluators_later(('tune_result', lambda: self._evalutate_tune_result(**{'tune_obj': self.tune_obj})))

    def get_static_locked(self):
        return self.static_locked

    def set_static_locked(self, static_locked):
        self.static_locked = static_locked
        self._static_locked_static_text.set_value(self.static_locked)

    def get_show_stream_tags_chk(self):
        return self.show_stream_tags_chk

    def set_show_stream_tags_chk(self, show_stream_tags_chk):
        self.show_stream_tags_chk = show_stream_tags_chk
        self._show_stream_tags_chk_check_box.set_value(self.show_stream_tags_chk)
        self.blocks_tag_debug_0.set_display(self.show_stream_tags_chk)

    def get_show_max_lvl(self):
        return self.show_max_lvl

    def set_show_max_lvl(self, show_max_lvl):
        self.show_max_lvl = show_max_lvl
        self._show_max_lvl_check_box.set_value(self.show_max_lvl)

    def get_show_max_freq(self):
        return self.show_max_freq

    def set_show_max_freq(self, show_max_freq):
        self.show_max_freq = show_max_freq
        self._show_max_freq_check_box.set_value(self.show_max_freq)

    def get_selected_gain_proxy(self):
        return self.selected_gain_proxy

    def set_selected_gain_proxy(self, selected_gain_proxy):
        self.selected_gain_proxy = selected_gain_proxy
        self.src.set_gain(self.selected_gain_proxy, 0)
        self.src.set_gain(self.selected_gain_proxy, 1)

    def get_selected_antenna_proxy(self):
        return self.selected_antenna_proxy

    def set_selected_antenna_proxy(self, selected_antenna_proxy):
        self.selected_antenna_proxy = selected_antenna_proxy
        self.src.set_antenna(self.selected_antenna_proxy, 0)
        self.src.set_antenna(self.selected_antenna_proxy, 1)

    def get_scope_mode_2(self):
        return self.scope_mode_2

    def set_scope_mode_2(self, scope_mode_2):
        self.scope_mode_2 = scope_mode_2
        self._scope_mode_2_chooser.set_value(self.scope_mode_2)
        self.blks2_selector_0_0_0.set_output_index(int(self.scope_mode_2))
        self.blks2_selector_0_1.set_input_index(int(self.scope_mode_2))

    def get_scope_mode(self):
        return self.scope_mode

    def set_scope_mode(self, scope_mode):
        self.scope_mode = scope_mode
        self._scope_mode_chooser.set_value(self.scope_mode)
        self.blks2_selector_0.set_input_index(int(self.scope_mode))
        self.blks2_selector_0_0.set_output_index(int(self.scope_mode))

    def get_requested_sample_rate(self):
        return self.requested_sample_rate

    def set_requested_sample_rate(self, requested_sample_rate):
        self.requested_sample_rate = requested_sample_rate
        self._run_evaluators_later(('actual_sample_rate', lambda: self._evalutate_actual_sample_rate(**{'requested_sample_rate': self.requested_sample_rate})))
        self.src.set_samp_rate(self.requested_sample_rate)

    def get_motherboard_sensor_names(self):
        return self.motherboard_sensor_names

    def set_motherboard_sensor_names(self, motherboard_sensor_names):
        self.motherboard_sensor_names = motherboard_sensor_names
        self._run_evaluators_later(('motherboard_sensors', lambda: self._evalutate_motherboard_sensors(**{'motherboard_sensor_names': self.motherboard_sensor_names})))

    def get_max_decim(self):
        return self.max_decim

    def set_max_decim(self, max_decim):
        self.max_decim = max_decim

    def get_initial_gain(self):
        return self.initial_gain

    def set_initial_gain(self, initial_gain):
        self.initial_gain = initial_gain
        self._run_evaluators_later(('gain_default', lambda: self._evalutate_gain_default(**{'initial_gain': self.initial_gain})))

    def get_has_lo_locked(self):
        return self.has_lo_locked

    def set_has_lo_locked(self, has_lo_locked):
        self.has_lo_locked = has_lo_locked
        self.set_locked_probe_poll_rate(self.lo_check_interval * float(self.has_lo_locked))

    def get_freq_range(self):
        return self.freq_range

    def set_freq_range(self, freq_range):
        self.freq_range = freq_range

    def get_fix_invalid_freq(self):
        return self.fix_invalid_freq

    def set_fix_invalid_freq(self, fix_invalid_freq):
        self.fix_invalid_freq = fix_invalid_freq

    def get_fft_max_lvl_value_2(self):
        return self.fft_max_lvl_value_2

    def set_fft_max_lvl_value_2(self, fft_max_lvl_value_2):
        self.fft_max_lvl_value_2 = fft_max_lvl_value_2

    def get_fft_max_lvl_value(self):
        return self.fft_max_lvl_value

    def set_fft_max_lvl_value(self, fft_max_lvl_value):
        self.fft_max_lvl_value = fft_max_lvl_value

    def get_fft_ave_probe(self):
        return self.fft_ave_probe

    def set_fft_ave_probe(self, fft_ave_probe):
        self.fft_ave_probe = fft_ave_probe
        self.logpwrfft_x_0.set_avg_alpha(self.fft_ave_probe)
        self.logpwrfft_x_0_0.set_avg_alpha(self.fft_ave_probe)

    def get_daughterboard_sensor_names(self):
        return self.daughterboard_sensor_names

    def set_daughterboard_sensor_names(self, daughterboard_sensor_names):
        self.daughterboard_sensor_names = daughterboard_sensor_names
        self._run_evaluators_later(('daughterboard_sensors', lambda: self._evalutate_daughterboard_sensors(**{'daughterboard_sensor_names': self.daughterboard_sensor_names})))

    def get_clicked_freq_txt(self):
        return self.clicked_freq_txt

    def set_clicked_freq_txt(self, clicked_freq_txt):
        self.clicked_freq_txt = clicked_freq_txt
        self._clicked_freq_txt_text_box.set_value(self.clicked_freq_txt)

    def get_auto_iq_correction(self):
        return self.auto_iq_correction

    def set_auto_iq_correction(self, auto_iq_correction):
        self.auto_iq_correction = auto_iq_correction
        self._auto_iq_correction_check_box.set_value(self.auto_iq_correction)
        self._run_evaluators_later(('variable_any_code_iq_correction', lambda: self._evalutate_variable_any_code_iq_correction(**{'auto_iq_correction': self.auto_iq_correction})))

    def get_auto_dc_offset_removal(self):
        return self.auto_dc_offset_removal

    def set_auto_dc_offset_removal(self, auto_dc_offset_removal):
        self.auto_dc_offset_removal = auto_dc_offset_removal
        self._auto_dc_offset_removal_check_box.set_value(self.auto_dc_offset_removal)
        self._run_evaluators_later(('variable_any_code_auto_dc_offset_removal', lambda: self._evalutate_variable_any_code_auto_dc_offset_removal(**{'auto_dc_offset_removal': self.auto_dc_offset_removal})))

    def get_any_test_1(self):
        return self.any_test_1

    def set_any_test_1(self, any_test_1):
        self.any_test_1 = any_test_1


if __name__ == '__main__':
    parser = OptionParser(option_class=eng_option, usage="%prog: [options]")
    parser.add_option("-A", "--antenna", dest="antenna", type="string", default="",
        help="Set Antenna (blank for default) [default=%default]")
    parser.add_option("-a", "--args", dest="args", type="string", default="",
        help="Set UHD device args [default=%default]")
    parser.add_option("", "--ave", dest="ave", type="eng_float", default=eng_notation.num_to_str(1*0 + 0.5),
        help="Set Average FFT [default=%default]")
    parser.add_option("", "--averaging", dest="averaging", type="string", default="True",
        help="Set Enable FFT averaging [default=%default]")
    parser.add_option("", "--bw", dest="bw", type="eng_float", default=eng_notation.num_to_str(0),
        help="Set Daughterboard bandwidth (where appropriate) [default=%default]")
    parser.add_option("", "--dyn-rng", dest="dyn_rng", type="eng_float", default=eng_notation.num_to_str(130),
        help="Set Dynamic Range [default=%default]")
    parser.add_option("", "--fft-rate", dest="fft_rate", type="intx", default=15,
        help="Set FFT Rate [default=%default]")
    parser.add_option("", "--fft-ref-scale", dest="fft_ref_scale", type="eng_float", default=eng_notation.num_to_str(2.0),
        help="Set FFT Ref Scale (p2p) [default=%default]")
    parser.add_option("", "--fft-size", dest="fft_size", type="intx", default=1024,
        help="Set FFT Size [default=%default]")
    parser.add_option("-f", "--freq", dest="freq", type="eng_float", default=eng_notation.num_to_str(0 + 100e6),
        help="Set Frequency [default=%default]")
    parser.add_option("", "--freq-fine-range", dest="freq_fine_range", type="eng_float", default=eng_notation.num_to_str(2e6),
        help="Set Fine frequency slider range [default=%default]")
    parser.add_option("-g", "--gain", dest="gain", type="eng_float", default=eng_notation.num_to_str(float("-inf")),
        help="Set Gain (default '-inf' selects relative gain of 25%) [default=%default]")
    parser.add_option("", "--lo-check-interval", dest="lo_check_interval", type="eng_float", default=eng_notation.num_to_str(5),
        help="Set LO lock check frequency (Hz) [default=%default]")
    parser.add_option("", "--lo-offset", dest="lo_offset", type="eng_float", default=eng_notation.num_to_str(0),
        help="Set LO offset (selects LO offset tuning mode) [default=%default]")
    parser.add_option("", "--mag-alpha", dest="mag_alpha", type="eng_float", default=eng_notation.num_to_str(1e-3),
        help="Set Signal magnitude probe alpha [default=%default]")
    parser.add_option("", "--peak-hold", dest="peak_hold", type="string", default="False",
        help="Set FFT peak hold [default=%default]")
    parser.add_option("", "--pps", dest="pps", type="string", default='',
        help="Set Time source (none, internal, external, mimo, gpsdo). <empty> leaves it at the default. [default=%default]")
    parser.add_option("", "--probe-interval", dest="probe_interval", type="eng_float", default=eng_notation.num_to_str(3),
        help="Set Signal probe frequency (Hz) [default=%default]")
    parser.add_option("-s", "--rate", dest="rate", type="eng_float", default=eng_notation.num_to_str(1e6),
        help="Set Sample Rate [default=%default]")
    parser.add_option("", "--ref", dest="ref", type="string", default='',
        help="Set Clock source (internal, external, mimo, gpsdo). <empty> leaves it at the default. [default=%default]")
    parser.add_option("", "--ref-lvl", dest="ref_lvl", type="eng_float", default=eng_notation.num_to_str(0),
        help="Set Reference Level [default=%default]")
    parser.add_option("", "--sensor-interval", dest="sensor_interval", type="eng_float", default=eng_notation.num_to_str(2),
        help="Set Sensor update frequency (Hz) [default=%default]")
    parser.add_option("", "--show-stream-tags", dest="show_stream_tags", type="string", default="False",
        help="Set Print stream tags [default=%default]")
    parser.add_option("", "--spec", dest="spec", type="string", default='',
        help="Set Sub-device specification (where appropriate) [default=%default]")
    parser.add_option("", "--stream-args", dest="stream_args", type="string", default="",
        help="Set Stream arguments (e.g. scalar=1024) [default=%default]")
    parser.add_option("", "--window", dest="window", type="string", default="auto",
        help="Set Window (bh: Blackman-Harris, ham: Hamming, han: Hanning, rect: Rectangular, flat: Flattop) [default=%default]")
    parser.add_option("", "--wire-format", dest="wire_format", type="string", default="",
        help="Set Wire format (e.g. sc16, sc8) [blank = automatic] [default=%default]")
    (options, args) = parser.parse_args()
    if gr.enable_realtime_scheduling() != gr.RT_OK:
        print "Error: failed to enable realtime scheduling."
    tb = mega_fft_2ch(antenna=options.antenna, args=options.args, ave=options.ave, averaging=options.averaging, bw=options.bw, dyn_rng=options.dyn_rng, fft_rate=options.fft_rate, fft_ref_scale=options.fft_ref_scale, fft_size=options.fft_size, freq=options.freq, freq_fine_range=options.freq_fine_range, gain=options.gain, lo_check_interval=options.lo_check_interval, lo_offset=options.lo_offset, mag_alpha=options.mag_alpha, peak_hold=options.peak_hold, pps=options.pps, probe_interval=options.probe_interval, rate=options.rate, ref=options.ref, ref_lvl=options.ref_lvl, sensor_interval=options.sensor_interval, show_stream_tags=options.show_stream_tags, spec=options.spec, stream_args=options.stream_args, window=options.window, wire_format=options.wire_format)
    tb.Start(True)
    tb.Wait()
